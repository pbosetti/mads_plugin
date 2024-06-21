/*
  _____ _ _ _
 |  ___(_) | |_ ___ _ __
 | |_  | | | __/ _ \ '__|
 |  _| | | | ||  __/ |
 |_|   |_|_|\__\___|_|

Base class for filter plugins
*/
#ifndef FILTER_HPP
#define FILTER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "common.hpp"

#ifdef _WIN32
#define EXPORTIT __declspec(dllexport)
#else
#define EXPORTIT
#endif

/*!
 * Base class for filters
 *
 * This class is the base class for all filters. It defines the interface for
 * loading data and processing it.
 * Derived classes must implement Filter::kind, Filter::load_data and 
 * Filter::process methods.
 * Optionally, they can implement the Filter::set_params method to receive 
 * parameters as a void pointer.
 * 
 * After deriving the class, remember to call the
 * #INSTALL_FILTER_DRIVER(klass, type_in, type_out) macro
 * to enable the plugin to be loaded by the kernel.
 *
 * @tparam Tin Input data type
 * @tparam Tout Output data type
 */
template <typename Tin = std::vector<double>,
          typename Tout = std::vector<double>>
class Filter {
public:
  Filter() : _error("No error"), dummy(false) {}
  virtual ~Filter() {}

  /*!
   * Returns the kind of filter
   *
   * This method returns the kind of filter. It is used to identify the filter
   * when loading it from a plugin.
   *
   * @return The kind of filter
   */
  virtual std::string kind() = 0;

  /*!
   * Loads the input data
   *
   * This method loads the input data into the filter. It returns true if the
   * data was loaded successfully, and false otherwise.
   *
   * @param data The input data
   * @return True if the data was loaded successfully, and false otherwise
   */
  virtual return_type load_data(Tin const &data) = 0;

  /*!
   * Processes the input data
   *
   * This method processes the input data and returns the result. It returns
   * true if the data was processed successfully, and false otherwise.
   *
   * @param out The output data
   * @return True if the data was processed successfully, and false otherwise
   */
  virtual return_type process(Tout &out) = 0;

  /*!
   * Sets the parameters
   *
   * This method sets the parameters for the filter. It receives a void pointer
   * to the parameters. The child class must cast the pointer to the correct
   * type.
   *
   * @param params The parameters (typically a pointer to a struct)
   */
  virtual void set_params(void const *params){
    _params = *(nlohmann::json *)params; 
    try {
      _agent_id = _params["agent_id"];
    } catch (nlohmann::json::exception &e) {
      _agent_id = "";
    }
  };

  /*!
   * Returns the filter information
   *
   * This method returns the filter information. It returns a map with keys and
   * values describing the filter.
   *
   * @return The filter information
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
  static const std::string server_name() { return "FilterServer"; }

protected:
  std::string _error;
  std::string _agent_id;
  nlohmann::json _params;
};

#ifndef HAVE_MAIN
#include <pugg/Driver.h>

/// @cond SKIP
template <typename Tin = std::vector<double>,
          typename Tout = std::vector<double>>
class FilterDriver : public pugg::Driver {
public:
  FilterDriver(std::string name, int version)
      : pugg::Driver(Filter<Tin, Tout>::server_name(), name, version) {}
  virtual Filter<Tin, Tout> *create() = 0;
};
// @endcond

#endif

#endif // FILTER_HPP