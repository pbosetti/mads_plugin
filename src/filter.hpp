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
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <chrono>
#include "common.hpp"

template <typename Tin = std::vector<double>,
          typename Tout = std::vector<double>>
class FilterDriver;

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
 * #MADS_REGISTER_PLUGINS(...) macro
 * to enable the plugin to be loaded by the kernel.
 *
 * @tparam Tin Input data type
 * @tparam Tout Output data type
 */
template <typename Tin = std::vector<double>,
          typename Tout = std::vector<double>>
class Filter {
public:
  /*!
   * The base type of this plugin class, used by mads::PluginDriver.
   */
  using plugin_base = Filter;

  /*!
   * The driver type matching this plugin class, used by mads::PluginDriver.
   */
  using driver_type = FilterDriver<Tin, Tout>;

  Filter() : dummy(false), _error("No error")  {}
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
   * @param topic The topic of the data
   * @param blob Pointer to binary data object (received)
   * @return True if the data was loaded successfully, and false otherwise
   */
  virtual return_type load_data(Tin const &data, std::string topic = "", std::vector<unsigned char> const *blob = nullptr) = 0;

  /*!
   * Processes the input data
   *
   * This method processes the input data and returns the result. It returns
   * true if the data was processed successfully, and false otherwise.
   *
   * @param out The output data
   * @param blob Pointer to binary data object (produced)
   * @return True if the data was processed successfully, and false otherwise
   */
  virtual return_type process(Tout &out, std::vector<unsigned char> *blob = nullptr) = 0;

  /*!
   * Loads the input data, with the blob given as a read-only view
   *
   * Called by the host instead of load_data() when the incoming blob lives
   * in a shared-memory ring (protocol V9). Calls without a blob still go
   * through load_data().
   *
   * The default implementation copies the view into the protected member
   * #_blob_copy and forwards to load_data(data, topic, &_blob_copy) (or with
   * a null blob pointer if `blob.data` is null), so plugins written for
   * earlier protocols keep working unchanged, at the cost of one copy. The
   * copy stays valid until the next load_data_view() call, i.e. through the
   * following process() call, as with the host-owned vector of the copying
   * path.
   *
   * Override it for zero-copy input. In that case observe the BlobView
   * contract: the view is read-only; it is valid only until the next
   * load_data()/load_data_view() call and only while the producer has not
   * overwritten the slot (the ring is lossy: the host re-checks the slot
   * after the call(s) and counts and discards torn reads); data to be kept
   * must be copied.
   *
   * @param data The input data
   * @param topic The topic of the data
   * @param blob Read-only view of the binary data object (received)
   * @return The outcome of the operation, as for load_data()
   */
  virtual return_type load_data_view(Tin const &data, std::string topic,
                                     BlobView blob) {
    if (blob.data == nullptr) {
      _blob_copy.clear();
      return load_data(data, std::move(topic), nullptr);
    }
    _blob_copy.assign(blob.data, blob.data + blob.size);
    return load_data(data, std::move(topic), &_blob_copy);
  }

  /*!
   * Tells whether the plugin can write its blob directly into a host buffer
   *
   * Override and return true to opt in to zero-copy output (protocol V9).
   * When this returns true and the agent has shared memory enabled, the host
   * calls process_shared() instead of process(). The value is queried once,
   * after set_params(), and must not change afterwards.
   *
   * @return True if process_shared() is implemented
   */
  virtual bool supports_shared_output() const { return false; }

  /*!
   * Processes the input data, writing the blob into a host-provided buffer
   *
   * Zero-copy variant of process(), called by the host instead of it when
   * supports_shared_output() returns true and the agent has shared memory
   * enabled. The plugin fills `out` exactly as in process(), writes at most
   * `buf.capacity` bytes of blob into `buf.data` and sets `buf.size` (0 means
   * no blob). Return codes have the same meaning as in process().
   *
   * If the blob does not fit, the plugin writes nothing, sets `buf.size` to
   * the required size (so that `buf.size > buf.capacity`), leaves its loaded
   * input untouched, and returns return_type::retry: the host then calls
   * process() on the same loaded input in the same loop iteration, to deliver
   * that payload through the copying path. See BlobBuffer for the full
   * contract.
   *
   * The default implementation returns return_type::error, and is never
   * called unless supports_shared_output() is overridden.
   *
   * @param out The output data
   * @param buf Host-provided writable buffer (valid only during the call)
   * @return The outcome of the operation
   */
  virtual return_type process_shared(Tout &out, BlobBuffer &buf) {
    (void)out;
    (void)buf;
    return return_type::error;
  }

  /*!
   * Sets the parameters
   *
   * This method sets the parameters for the filter. It receives a void pointer
   * to the parameters. The child class must cast the pointer to the correct
   * type.
   *
   * @param params The parameters 
   */
  virtual void set_params(const nlohmann::json &params){
    _agent_id = params.value("agent_id", "");
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
  static const int version = PLUGIN_PROTOCOL_VERSION;

  /*!
   * Returns the plugin server name.
   */
  static std::string server_name() { return "FilterServer"; }

  /*!
   * The desired duration of current loop iteration
   */
  std::chrono::duration<long long, std::milli> next_loop_duration{0};

protected:
  std::string _error;
  std::string _agent_id;
  nlohmann::json _params;
  /*!
   * Copy of the last blob view, owned by the base class and filled by the
   * default load_data_view() implementation.
   */
  std::vector<unsigned char> _blob_copy;
};

#ifndef HAVE_MAIN
#include <pugg/Driver.h>

/// @cond SKIP
template <typename Tin, typename Tout>
class FilterDriver : public pugg::Driver {
public:
  FilterDriver(std::string name, int version)
      : pugg::Driver(Filter<Tin, Tout>::server_name(), std::move(name), version) {}
  virtual std::unique_ptr<Filter<Tin, Tout>> create() = 0;
};
// @endcond

#endif

#endif // FILTER_HPP