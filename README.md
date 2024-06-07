# Plugins!
![Build status](https://github.com/miroscic/plugin_cpp/actions/workflows/cmake-multi-platform.yml/badge.svg)

This example project explores how to develop a plugin system for a C++ application. It is based on the [pugg plugin system]().

## Building

To build the project, you need to have CMake installed. Then, you can run the following commands:

```bash
mkdir build
ccmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -t install
```

This creates three plugins in the form of executable files **that also export symbols**, so that they can be externally loaded by the main application. The plugins are named `echo.plugin`, `twice.plugin`, and `echoj.plugin`.

The latter plugin is a template for complete plugins, and it is pretty extensible. Indeed, it accepts a JSON object as settings, a JSON object as input, and produces a JSON object as output. The other two plugins are simple examples of how to implement a plugin.

Plugins are named **Filters**, for they are expected to act as filters, taking an input and producing an output. The plugins must be implemented as derived classes of the templated class `Filter` (see `src/filter.hpp`).

If needed, this project will be extended by adding base classes for other types of plugins, such as **Sources** (output only) and **Sinks** (input only).

## Executing

The install step creates and populates the `usr` directory in the project root folder. You can run the executable that loads the plugin with the following command:

```bash
cd usr
bin/loaderj bin/echoj.plugin
```

Note that **on MacOS only** the `echoj.plugin` file is actually an executable file that exports symbols. It is not a shared library, but it is a plugin that can also be directly executed (using its internal `main()`) as:

```bash
bin/echoj.plugin
```

This is a very flexible way for implementing standalone apps that can also be used as plugins within the Miroscic framework of distributed agents.

On Windows and Linux, the plugin is a shared library that can only be loaded by the `loader` executable. Beside the plugin, an equivalent executable is also generated.


## Plugin Versioning

The plugin system uses an internal version number `Filter::version` to check compatibility between the main application and the plugins. To invalidate a previously released plugin, simply imcrease the version number in the base class.

## Derived classes

To create a new plugin, fork this repository and implement a derived class of `Filter` or `Source` in a new file. At the end of the new derived class definition, add the macros that set up the plugin driver: if it is a source, add

```cpp
INSTALL_SOURCE_DRIVER(MySourceClassName, json)
```

If it is a filter, add

```cpp
INSTALL_FILTER_DRIVER(MyFilterClassName, json, json)
```

If it is a sink, add

```cpp
INSTALL_SINK_DRIVER(MySinkClassName, json)
```

Finally, create a new target in the `CMakeLists.txt` file that compiles the new plugin. Something like:

```cmake
add_plugin(webcam LIBS LibsNeeded)
```