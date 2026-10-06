# Plugins for the MADS network of distributed agents
![Build status](https://github.com/pbosetti/mads_plugin/actions/workflows/cmake-multi-platform.yml/badge.svg)

This example project explores how to develop a plugin system for a C++ application. It is based on the [pugg plugin system]().

## Building

The project is primarily consumed with `FetchContent` as a header-only interface
target, `MADS::Plugin`, which exposes the public headers in `src/`:

* `common.hpp`
* `datastore.hpp`
* `source.hpp`
* `filter.hpp`
* `sink.hpp`

In a consuming CMake project:

```cmake
include(FetchContent)

FetchContent_Declare(mads_plugin
  GIT_REPOSITORY https://github.com/pbosetti/mads_plugin.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(mads_plugin)

target_link_libraries(your_target PRIVATE MADS::Plugin)
```

The equivalent `mads_plugin::mads_plugin` target alias is also available.

Example targets are disabled by default for FetchContent users. To build them,
set `PLUGIN_ENABLE_EXAMPLES=ON` before `FetchContent_MakeAvailable()`.

To build the package without examples, you need to have CMake installed. Then,
you can run the following commands:

```bash
mkdir build
ccmake -Bbuild -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build -j 8
sudo cmake --build build -t install
```

To build the example loaders and plugins, enable `PLUGIN_ENABLE_EXAMPLES`:

```bash
cmake -Bbuild -DPLUGIN_ENABLE_EXAMPLES=ON
cmake --build build -j 8
```

Plugins can be runtime loaded by MADS agents as `mads-source`, `mads-filter`, and `mads-sink` executables. The plugins are compiled as shared libraries, and they are installed in the `<install prefix>/lib` directory.

There can be three types of plugins: **sources**, **filters**, and **sinks**, suitable to be loaded by the corresponding MADS agents.

In the `examples/plugin` directory there are three templates for the three types of plugins, plus some example plugins, such as `echoj`, `clock`, and `running_avg`.

Typically, each plugin code can contain a conditionally available `main()` function that can be used to test the plugin as a standalone executable. This is useful for debugging and testing the plugin before integrating it into the MADS framework. On MacOS, the plugin can be executed as a standalone executable, while on Linux and Windows, it can only be loaded by the corresponding agent executable. On the latter platforms, the plugin is also compiled as an executable that can be run directly. For example, the `clock.cpp` source is compiled on Linux and Windows as the library `clock.plugin` and the executable `clock`.


## Plugin Versioning

The plugin system uses an internal version number `Filter::version` to check compatibility between the main application and the plugins. When loaded, the version number are checked and if the plugin protocol version is lower than that used by the plugin loader (i.e. one of the MADS commands), the loading fails.

When this happens, you have to:

* delete the content of `build\_deps\plugin_src`
* update the version of the plugin in the proper `FetchContent_Populate` command in `CMakeLists.txt`: for example if your current MADS version 2.x says that the minimum plugin protocolo version is 6, then you have to look at <https://github.com/pbosetti/mads_plugin> and find the latest tag ending in `P6` (e.g. v2.0-P6), and replace the value `GIT_TAG` with that new tag
* recompile (and possibly reinstall) the plugin

### Protocol V9: shared-memory (zero-copy) blobs

Protocol V9 prepares plugins for the MADS shared-memory data plane, where the
host agent owns a ring of fixed-size slots in shared memory and only
descriptors travel over ZeroMQ. `common.hpp` adds two ABI-plain structs:

* `BlobView { const unsigned char *data; size_t size; }`: a **read-only** view
  of an incoming blob in the producer's ring;
* `BlobBuffer { unsigned char *data; size_t capacity; size_t size; }`: a
  writable, host-provided buffer (a ring slot) for an outgoing blob.

and the following virtual methods, all with default implementations:

| Class    | New method | Default |
|----------|------------|---------|
| `Source` | `bool supports_shared_output() const` | `false` |
| `Source` | `return_type get_output_shared(Tout &out, BlobBuffer &buf)` | `return_type::error` (never called unless opted in) |
| `Filter` | `return_type load_data_view(Tin const &data, std::string topic, BlobView blob)` | copies the view into `_blob_copy`, then calls `load_data()` |
| `Filter` | `bool supports_shared_output() const` | `false` |
| `Filter` | `return_type process_shared(Tout &out, BlobBuffer &buf)` | `return_type::error` (never called unless opted in) |
| `Sink`   | `return_type load_data_view(Tin const &data, std::string topic, BlobView blob)` | copies the view into `_blob_copy`, then calls `load_data()` |

**Existing plugins need no source changes**: just recompile them against V9.
Input blobs reach them through the default `load_data_view()`, which makes one
copy and forwards to the usual `load_data()`; output keeps going through
`get_output()`/`process()`.

To opt in to zero-copy:

* **input** (filters and sinks): override `load_data_view()` and read
  `blob.data` directly. The view is read-only; it is valid only until the next
  `load_data()`/`load_data_view()` call (for filters: through the following
  `process()`), and only while the producer has not overwritten the slot. The
  ring is lossy: the host re-checks the slot after the call and discards (and
  counts) torn reads. Copy whatever you need to keep.
* **output** (sources and filters): return `true` from
  `supports_shared_output()` and implement `get_output_shared()` /
  `process_shared()`, filling `out` as usual, writing at most `buf.capacity`
  bytes into `buf.data` and setting `buf.size`. If the blob does not fit,
  write nothing, set `buf.size` to the required size (so that
  `buf.size > buf.capacity`) and return `return_type::retry` without consuming
  the pending data: the host then calls `get_output()`/`process()` in the same
  loop iteration and sends that payload through the copying path.

```cpp
bool supports_shared_output() const override { return true; }

return_type get_output_shared(json &out, BlobBuffer &buf) override {
  size_t n = _frame.size();
  if (n > buf.capacity) { buf.size = n; return return_type::retry; }
  std::memcpy(buf.data, _frame.data(), n); // or acquire straight into buf.data
  buf.size = n;
  out["size"] = n;
  return return_type::success;
}
```

## Implement new plugins

To create a new plugin, implement a derived class of `Filter`, `Source` or `Sink` by copying one of the templates. 

Finally, create a new target in the `examples/plugin/CMakeLists.txt` file that compiles the new plugin. Something like:

```cmake
add_plugin(webcam SRCS other/possibly/needed/source.cpp LIBS LibsNeeded)
```

The main `CMakeLists.txt` file will automatically detect the new plugin and compile it. This file **shall not** be modified, unless you know what you're doing.

# The `SerialPort` class

The repo also contains a portable C++20 class `SerialPort` that can be used to read and write data from/to a serial port. The class is implemented in `src/serialport.hpp` and it is used in the `examples/serialport.cpp` example.

The class is portable and tested on Windows, Linux, and macOS.

# License

Apache 2.0 License. See the LICENSE file for details.


# Authors

Paolo Bosetti (UniTN), with the help from Anna-Carla Araujo and Guillaume Cohen (INSA Toulouse)
