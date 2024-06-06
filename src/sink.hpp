/*
  ____  _       _    
 / ___|(_)_ __ | | __
 \___ \| | '_ \| |/ /
  ___) | | | | |   < 
 |____/|_|_| |_|_|\_\
                     
Base class for sink plugins
*/
#ifndef SINK_HPP
#define SINK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "common.hpp"

#ifdef _WIN32
#define EXPORTIT __declspec(dllexport)
#else
#define EXPORTIT
#endif

/*!
 * Base class for sinks
 *
 * This class is the base class for all sinks. It defines the interface for
 * loading data and processing it.
 * Derived classes must implement Sink::kind, Sink::load_data and 
 * Sink::process methods.
 * Optionally, they can implement the Sink::set_params method to receive 
 * parameters as a void pointer.
 * 
 * After deriving the class, remember to call the
 * #INSTALL_SINK_DRIVER(klass, type_in, type_out) macro
 * to enable the plugin to be loaded by the kernel.
 *
 * @tparam Tin Input data type
 * @tparam Tout Output data type
 */
template <typename Tin = std::vector<double>>
class Sink {
public:
  Sink() : _error("No error"), dummy(false) {}
  virtual ~Sink() {}

  /*!
   * Returns the kind of sink
   *
   * This method returns the kind of sink. It is used to identify the sink
   * when loading it from a plugin.
   *
   * @return The kind of sink
   */
  virtual std::string kind() = 0;

  /*!
   * Loads the input data
   *
   * This method loads the input data into the sink. It returns true if the
   * data was loaded successfully, and false otherwise.
   *
   * @param data The input data
   * @return True if the data was loaded successfully, and false otherwise
   */
  virtual return_type load_data(Tin &data) = 0;

  /*!
   * Sets the parameters
   *
   * This method sets the parameters for the sink. It receives a void pointer
   * to the parameters. The child class must cast the pointer to the correct
   * type.
   *
   * @param params The parameters (typically a pointer to a struct)
   */
  virtual void set_params(void *params){};

  /*!
   * Returns the sink information
   *
   * This method returns the sink information. It returns a map with keys and
   * values describing the sink.
   *
   * @return The sink information
   */
  virtual std::map<std::string, std::string> info() = 0;

  /*!
   * Returns the error message
   *
   * This method returns the error message.
   *
   * @return The error message
   */
  std::string error() { return _error; }

  /*!
   * Set it to true to enable dummy mode
  */
  bool dummy = false;

  /*!
   * Returns the plugin protocol version.
   */
  static const int version = 2;

  /*!
   * Returns the plugin server name.
   */
  static const std::string server_name() { return "SinkServer"; }

private:
  std::string _error;
};

#ifndef HAVE_MAIN
#include <pugg/Driver.h>

/// @cond SKIP
template <typename Tin = std::vector<double>>
class SinkDriver : public pugg::Driver {
public:
  SinkDriver(std::string name, int version)
      : pugg::Driver(Sink<Tin>::server_name(), name, version) {}
  virtual Sink<Tin> *create() = 0;
};
// @endcond

#endif

#endif // SINK_HPP