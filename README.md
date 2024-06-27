# Plugins for the MADS network of distributed agents
![Build status](https://github.com/pbosetti/mads_plugin/actions/workflows/cmake-multi-platform.yml/badge.svg)

This example project explores how to develop a plugin system for a C++ application. It is based on the [pugg plugin system]().

## Building

To build the project, you need to have CMake installed. Then, you can run the following commands:

```bash
mkdir build
ccmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -t install
```

Plugins can be runtime loaded by MADS agents as `mads-source`, `mads-filter`, and `mads-sink` executables. The plugins are compiled as shared libraries, and they are installed in the `<install prefix>/lib` directory.

There can be three types of plugins: **sources**, **filters**, and **sinks**, suitable to be loaded by the corresponding MADS agents.

In the `src/plugins` directory there are three templates for the three types of plugins, plus some example plugins, such as `echo`, `twice`, and `echoj`.

Typically, each plugin code can contain a conditionally available `main()` function that can be used to test the plugin as a standalone executable. This is useful for debugging and testing the plugin before integrating it into the MADS framework. On MacOS, the plugin can be executed as a standalone executable, while on Linux and Windows, it can only be loaded by the corresponding agent executable. On the latter platforms, the plugin is also compiled as an executable that can be run directly. For example, the `clock.cpp` source is compiled on Linux and Windows as the library `clock.plugin` and the executable `clock`.


## Plugin Versioning

The plugin system uses an internal version number `Filter::version` to check compatibility between the main application and the plugins. To invalidate a previously released plugin, simply imcrease the version number in the base class.

## Implement new plugins

To create a new plugin, implement a derived class of `Filter`, `Source` or `Sink` by copying one of the templates. 

Finally, create a new target in the `src/plugin/CMakeLists.txt` file that compiles the new plugin. Something like:

```cmake
add_plugin(webcam SRCS other/possibly/needed/source.cpp LIBS LibsNeeded)
```

The main `CMakeLists.txt` file will automatically detect the new plugin and compile it. This file **shall not** be modified, unless you know what you're doing.


# Authors

Paolo Bosetti (UniTN), with the help from Anna-Carla Araujo and Guillaume Cohen (INSA Toulouse)