/*
 __        __         _                     _             _       
 \ \      / /__  _ __| | _____ _ __   _ __ | |_   _  __ _(_)_ __  
  \ \ /\ / / _ \| '__| |/ / _ \ '__| | '_ \| | | | |/ _` | | '_ \ 
   \ V  V / (_) | |  |   <  __/ |    | |_) | | |_| | (_| | | | | |
    \_/\_/ \___/|_|  |_|\_\___|_|    | .__/|_|\__,_|\__, |_|_| |_|
                                     |_|            |___/         
This is a worker plugin to show how to use the dealer/worker pattern
*/
// Mandatory included headers
#include "../filter.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <chrono>
#include <thread>
// other includes as needed here

// Define the name of the plugin
#ifndef PLUGIN_NAME
#define PLUGIN_NAME "worker"
#endif

// Load the namespaces
using namespace std;
using json = nlohmann::json;


// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class WorkerPlugin : public Filter<json, json> {

public:

  string kind() override { return PLUGIN_NAME; }

  return_type load_data(json const &input, string topic = "") override {
    if (!input.contains("data")) {
      _error = "No data field in input";
      return return_type::error;
    }
    _sleep_request = chrono::milliseconds(input["data"].value("period", 100));
    _id = input["data"].value("id", "");
    return return_type::success;
  }

  return_type process(json &out) override {
    out.clear();
    if (_sleep_request.count() < 0) {
      _error = "Invalid sleep period";
      return return_type::error;
    }
    const auto start = chrono::high_resolution_clock::now();
    this_thread::sleep_for(_sleep_request);
    const auto end = std::chrono::high_resolution_clock::now();
    const chrono::duration<double, std::milli> elapsed = end - start;  
    out["requested"] = _sleep_request.count();
    out["elapsed"] = elapsed.count();
    out["id"] = _id;
    if (!_agent_id.empty()) out["agent_id"] = _agent_id;
    return return_type::success;
  }
  
  void set_params(void const *params) override {
    Filter::set_params(params);
    _params.merge_patch(*(json *)params);
  }

  map<string, string> info() override { 
    return {}; 
  };

private:
  chrono::milliseconds _sleep_request;
  string _id;
};


/*
  ____  _             _             _      _
 |  _ \| |_   _  __ _(_)_ __     __| |_ __(_)_   _____ _ __
 | |_) | | | | |/ _` | | '_ \   / _` | '__| \ \ / / _ \ '__|
 |  __/| | |_| | (_| | | | | | | (_| | |  | |\ V /  __/ |
 |_|   |_|\__,_|\__, |_|_| |_|  \__,_|_|  |_| \_/ \___|_|
                |___/
Enable the class as plugin
*/
INSTALL_FILTER_DRIVER(WorkerPlugin, json, json);


/*
                  _       
  _ __ ___   __ _(_)_ __  
 | '_ ` _ \ / _` | | '_ \ 
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|
                          
*/

int main(int argc, char const *argv[])
{
  WorkerPlugin plugin;
  json params;
  json input, output;

  // Set example values to params
  params["test"] = "value";

  // Set the parameters
  plugin.set_params(&params);

  // Set input data
  input["data"] = {
    {"period", 1000}
  };

  // Set input data
  plugin.load_data(input);
  cout << "Input: " << input.dump(2) << endl;

  // Process data
  plugin.process(output);
  cout << "Output: " << output.dump(2) << endl;


  return 0;
}

