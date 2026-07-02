/*
  ____                           
 / ___|  ___  _   _ _ __ ___ ___ 
 \___ \ / _ \| | | | '__/ __/ _ \
  ___) | (_) | |_| | | | (_|  __/
 |____/ \___/ \__,_|_|  \___\___|
                                 
Base class for source plugins
*/
#ifndef SOURCE_HPP
#define SOURCE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>
#include "common.hpp"

template <typename Tout = std::vector<double>> class SourceDriver;

/*!
 * Base class for sources
 *
 * This class is the base class for all sources. It defines the interface for
 * providing output of data internally acquired.
 * Child classes must implement the Source::kind() and Source::get_output()
 * methods.
 * Optionally, they can implement the Source::set_params() method to receive
 * parameters as a void pointer.
 *
 * After deriving the class, remember to call the
 * #MADS_REGISTER_PLUGINS(...) macro to enable the plugin to be loaded
 * by the kernel.
 *
 * @tparam Tout Output data type
 */
template <typename Tout = std::vector<double>>
class Source {
public:
  /*!
   * The base type of this plugin class, used by mads::PluginDriver.
   */
  using plugin_base = Source;

  /*!
   * The driver type matching this plugin class, used by mads::PluginDriver.
   */
  using driver_type = SourceDriver<Tout>;

  Source() : _blob_format("none"), _error("No error"), _agent_id("") {}
  virtual ~Source() {}

  /*!
   * Returns the kind of source
   *
   * This method returns the kind of source. It is used to identify the source
   * when loading it from a plugin.
   *
   * @return The kind of source
   */
  virtual std::string kind() = 0;

  /*!
   * Get the output data
   *
   * This method provides the output data. It returns
   * true if the data was fetched successfully, and false otherwise.
   *
   * @param out The output data
   * @param blob Pointer to binary data object (produced)
   * @return True if the data was processed successfully, and false otherwise
   */
  virtual return_type get_output(Tout &out, std::vector<unsigned char> *blob = nullptr) = 0;


  /*!
   * Sets the parameters
   *
   * This method sets the parameters for the source. It receives a void pointer
   * to the parameters. The derived class must cast the pointer to the correct
   * type.
   * 
   * Derived classes shall call the parent class method to set the `_agent_id`
   * field in the `_params` json object.
   *
   * @param params The parameters 
   */
  virtual void set_params(const nlohmann::json &params){
    _agent_id = params.value("agent_id", "");
  };

  /*!
   * Returns the source information
   *
   * This method returns the source information. It returns a map with keys and
   * values describing the source.
   *
   * @return The source information
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
   * Returns the format of the blob data provided by get_output().
   */
  std::string blob_format() { return _blob_format; }

  /*!
   * Set it to true to enable dummy mode
   */
  bool dummy = false;


  static const int version = PLUGIN_PROTOCOL_VERSION;

  /*!
   * Returns the plugin server name.
   */
  static std::string server_name() { return "SourceServer"; }
  
  /*!
   * The desired duration of current loop iteration
   */
  std::chrono::duration<long long, std::milli> next_loop_duration{0};

protected:
  nlohmann::json _params;
  std::string _blob_format;
  std::string _error;
  std::string _agent_id;
};

#ifndef HAVE_MAIN
#include <pugg/Driver.h>

/// @cond SKIP
template <typename Tout>
class SourceDriver : public pugg::Driver {
public:
  SourceDriver(std::string name, int version)
      : pugg::Driver(Source<Tout>::server_name(), std::move(name), version) {}
  virtual std::unique_ptr<Source<Tout>> create() = 0;
};
/// @endcond

#endif

#endif // SOURCE_HPP
