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
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "common.hpp"

template <typename Tin = std::vector<double>> class SinkDriver;

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
 * #MADS_REGISTER_PLUGINS(...) macro
 * to enable the plugin to be loaded by the kernel.
 *
 * @tparam Tin Input data type
 */
template <typename Tin = std::vector<double>>
class Sink {
public:
  /*!
   * The base type of this plugin class, used by mads::PluginDriver.
   */
  using plugin_base = Sink;

  /*!
   * The driver type matching this plugin class, used by mads::PluginDriver.
   */
  using driver_type = SinkDriver<Tin>;

  Sink() : dummy(false), _error("No error") {}
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
   * @param topic The topic of the data
   * @param blob Pointer to binary data object (received)
   * @return True if the data was loaded successfully, and false otherwise
   */
  virtual return_type load_data(Tin const &data, std::string topic = "", std::vector<unsigned char> const *blob = nullptr) = 0;

  /*!
   * Sets the parameters
   *
   * This method sets the parameters for the sink. It receives a void pointer
   * to the parameters. The child class must cast the pointer to the correct
   * type.
   *
   * @param params The parameters 
   */
  virtual void set_params(const nlohmann::json &params){
    _agent_id = params.value("agent_id", "");
  };

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
  static const int version = PLUGIN_PROTOCOL_VERSION;

  /*!
   * Returns the plugin server name.
   */
  static std::string server_name() { return "SinkServer"; }

protected:
  std::string _error;
  std::string _agent_id;
  nlohmann::json _params;
};

#ifndef HAVE_MAIN
#include <pugg/Driver.h>

/// @cond SKIP
template <typename Tin>
class SinkDriver : public pugg::Driver {
public:
  SinkDriver(std::string name, int version)
      : pugg::Driver(Sink<Tin>::server_name(), std::move(name), version) {}
  virtual std::unique_ptr<Sink<Tin>> create() = 0;
};
// @endcond

#endif

#endif // SINK_HPP