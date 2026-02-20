#ifndef COMMON_HPP
#define COMMON_HPP

#define PLUGIN_PROTOCOL_VERSION 7

/*!
* @file common.hpp
* @brief Common definitions for the pugg library
*/


/*!
* @def INSTALL_SOURCE_DRIVER(klass, type)
* Install a source driver into the kernel.
* 
* @brief Call this macro after defining a source class to install it into the
* kernel.
* @param klass the class name
* @param type the output type of the source
*/
#define INSTALL_SOURCE_DRIVER(klass, type)                                     \
  class klass##Driver : public SourceDriver<type> {                            \
  public:                                                                      \
    klass##Driver() : SourceDriver(PLUGIN_NAME, klass::version) {}             \
    Source<type> *create() { return new klass(); }                             \
  };                                                                           \
  extern "C" EXPORTIT void register_pugg_plugin(pugg::Kernel *kernel) {        \
    kernel->add_driver(new klass##Driver());                                   \
  }

/*!
* \def INSTALL_FILTER_DRIVER(klass, type_in, type_out)
* 
* @brief Call this macro after defining a filter class to install it into the
* kernel.
* @param klass the class name
* @param type_in the input type of the filter
* @param type_out the output type of the filter
*/
#define INSTALL_FILTER_DRIVER(klass, type_in, type_out)                        \
  class klass##Driver : public FilterDriver<type_in, type_out> {               \
  public:                                                                      \
    klass##Driver() : FilterDriver(PLUGIN_NAME, klass::version) {}             \
    Filter<type_in, type_out> *create() { return new klass(); }                \
  };                                                                           \
  extern "C" EXPORTIT void register_pugg_plugin(pugg::Kernel *kernel) {        \
    kernel->add_driver(new klass##Driver());                                   \
  }

/*!
* @def INSTALL_SINK_DRIVER(klass, type)
* Install a sink driver into the kernel.
* 
* @brief Call this macro after defining a source class to install it into the
* kernel.
* @param klass the class name
* @param type the input type for the sink
*/
#define INSTALL_SINK_DRIVER(klass, type)                                     \
  class klass##Driver : public SinkDriver<type> {                            \
  public:                                                                      \
    klass##Driver() : SinkDriver(PLUGIN_NAME, klass::version) {}             \
    Sink<type> *create() { return new klass(); }                             \
  };                                                                           \
  extern "C" EXPORTIT void register_pugg_plugin(pugg::Kernel *kernel) {        \
    kernel->add_driver(new klass##Driver());                                   \
  }

/*!
* @brief The return type of common interface functions.
*/
enum class return_type {
  success = 0,
  retry,
  warning,
  error,
  critical
};


#endif // COMMON_HPP