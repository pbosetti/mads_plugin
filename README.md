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

## Implement new plugins

To create a new plugin, implement a derived class of `Filter`, `Source` or `Sink` by copying one of the templates. 

Finally, create a new target in the `examples/plugin/CMakeLists.txt` file that compiles the new plugin. Something like:

```cmake
add_plugin(webcam SRCS other/possibly/needed/source.cpp LIBS LibsNeeded)
```

The main `CMakeLists.txt` file will automatically detect the new plugin and compile it. This file **shall not** be modified, unless you know what you're doing.


# Authors

Paolo Bosetti (UniTN), with the help from Anna-Carla Araujo and Guillaume Cohen (INSA Toulouse)
