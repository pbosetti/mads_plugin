#ifndef COMMON_HPP
#define COMMON_HPP

#define PLUGIN_PROTOCOL_VERSION 8

/*!
 * @file common.hpp
 * @brief Common definitions for MADS plugins
 */

/*!
 * @def EXPORTIT
 * Marks the plugin registration function as visible outside the shared
 * library, so that the kernel can find it with dlsym()/GetProcAddress().
 */
#if defined(_WIN32)
#define EXPORTIT __declspec(dllexport)
#else
#define EXPORTIT __attribute__((visibility("default")))
#endif

/*!
 * @brief The return type of common interface functions.
 */
enum class return_type { success = 0, retry, warning, error, critical };

#ifndef HAVE_MAIN
#include <pugg/Kernel.h>
#include <memory>
#include <string>
#include <utility>

namespace mads {

/// @cond SKIP
/*!
 * Generic pugg driver for a plugin class.
 *
 * Works for any class deriving from Source, Filter or Sink: those bases
 * provide the plugin_base and driver_type aliases this template relies on,
 * so the input/output types never need to be repeated at registration time.
 *
 * @tparam P the concrete plugin class
 */
template <class P>
class PluginDriver final : public P::driver_type {
  using Base = typename P::driver_type;

public:
  explicit PluginDriver(std::string name)
      : Base(std::move(name), P::version) {}
  std::unique_ptr<typename P::plugin_base> create() override {
    return std::make_unique<P>();
  }
};
/// @endcond

/*!
 * Registers the given plugin classes into the kernel.
 *
 * Registration succeeds if at least one class is accepted: hosts typically
 * register only the server type they care about (source, filter or sink),
 * so classes of other types are expected to be rejected and are skipped.
 * If no class is accepted, the kernel unloads the shared library.
 *
 * Exceptions are swallowed and reported as a failed registration: nothing
 * may ever propagate across the plugin boundary.
 *
 * @tparam Plugins the plugin classes to register
 * @param kernel the kernel to register into
 * @param name the driver name (typically PLUGIN_NAME)
 * @return true if at least one plugin class was registered
 */
template <class... Plugins>
bool install(pugg::Kernel &kernel, const std::string &name) noexcept {
  try {
    // Bitwise | so that every class is attempted, without short-circuiting
    return (static_cast<bool>(kernel.add_driver(
                std::make_unique<PluginDriver<Plugins>>(name))) |
            ...);
  } catch (...) {
    return false;
  }
}

} // namespace mads

/*!
 * @def MADS_REGISTER_PLUGINS(...)
 * Install one or more plugin classes into the kernel.
 *
 * @brief Call this macro once per shared library, after defining the plugin
 * classes, to make them loadable by the kernel. The input/output types are
 * deduced from each class's base, so only the class names are needed:
 *
 * ```cpp
 * MADS_REGISTER_PLUGINS(Echo)           // a single plugin
 * MADS_REGISTER_PLUGINS(MySrc, MySink)  // several plugins in one library
 * ```
 *
 * Requires PLUGIN_NAME to be defined. When HAVE_MAIN is defined the macro
 * expands to nothing, so the same source compiles without pugg.
 */
#define MADS_REGISTER_PLUGINS(...)                                            \
  extern "C" EXPORTIT bool register_pugg_plugin(pugg::Kernel *kernel) {       \
    return kernel != nullptr &&                                               \
           mads::install<__VA_ARGS__>(*kernel, PLUGIN_NAME);                  \
  }

#else // HAVE_MAIN: build without pugg, registration compiles away

#define MADS_REGISTER_PLUGINS(...)

#endif // HAVE_MAIN

/*!
 * @name Deprecated registration macros
 * Kept for backward compatibility; the type arguments are ignored, since
 * MADS_REGISTER_PLUGINS() deduces them from the class itself.
 */
///@{
#define INSTALL_SOURCE_DRIVER(klass, type) MADS_REGISTER_PLUGINS(klass)
#define INSTALL_FILTER_DRIVER(klass, type_in, type_out) MADS_REGISTER_PLUGINS(klass)
#define INSTALL_SINK_DRIVER(klass, type) MADS_REGISTER_PLUGINS(klass)
///@}

#endif // COMMON_HPP
