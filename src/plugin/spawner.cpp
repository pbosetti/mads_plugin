/*
  ____                                     _             _             
 / ___| _ __   __ ___      ___ __    _ __ | |_   _  __ _(_)_ __        
 \___ \| '_ \ / _` \ \ /\ / / '_ \  | '_ \| | | | |/ _` | | '_ \       
  ___) | |_) | (_| |\ V  V /| | | | | |_) | | |_| | (_| | | | | |      
 |____/| .__/ \__,_| \_/\_/ |_| |_| | .__/|_|\__,_|\__, |_|_| |_|      
       |_|                          |_|            |___/               
A Template for a Source Plugin
*/
// Mandatory included headers
#include "../source.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <queue>
#include <chrono>
#include <thread>
#include <uuid/uuid.h>

// other includes as needed here

// Define the name of the plugin
#ifndef PLUGIN_NAME
#define PLUGIN_NAME "spawner"
#endif

// Load the namespaces
using namespace std;
using json = nlohmann::json;


// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class SpawnerPlugin : public Source<json> {

public:

  // Typically, no need to change this
  string kind() override { return PLUGIN_NAME; }

  // Implement the actual functionality here
  return_type get_output(json &out,
                         std::vector<unsigned char> *blob = nullptr) override {
    out.clear();
    if (_periods.empty()) {
      _error = "No more periods available";
      return return_type::critical;
    }
    size_t period = _periods.front();
    _periods.pop();
    out["data"]["period"] = period;


    // Generate a unique ID
    uuid_t id;
    uuid_generate(id);

    // Convert the ID to a string
    char id_str[37];
    uuid_unparse(id, id_str);

    // Assign the ID to the output JSON
    out["data"]["id"] = id_str;
    
    if (!_agent_id.empty()) out["agent_id"] = _agent_id;
    return return_type::success;
  }

  void set_params(const json &params) override {
    Source::set_params(params);
    _params["period_min"] = 100;
    _params["period_max"] = 5000;
    _params["number"] = 50;

    _params.merge_patch(params);
    _number = _params["number"];
    _period_min = _params["period_min"];
    _period_max = _params["period_max"];

    for (size_t i = 0; i < _number; i++) {
      size_t period = _period_min + (rand() % (_period_max - _period_min + 1));
      _periods.push(period);
    }
    this_thread::sleep_for(chrono::milliseconds(1000));
  }

  // Implement this method if you want to provide additional information
  map<string, string> info() override { 
    return {
      {"period_min", to_string(_period_min)},
      {"period_max", to_string(_period_max)},
      {"number", to_string(_number)},
    }; 
  };

private:
  size_t _period_min;
  size_t _period_max;
  size_t _number;
  queue<size_t> _periods;
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
INSTALL_SOURCE_DRIVER(SpawnerPlugin, json)


/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  SpawnerPlugin plugin;
  json output, params;
  // Set example values to params
  params["number"] = 10;
  params["period_min"] = 100;
  params["period_max"] = 5000;

  // Set the parameters
  plugin.set_params(params);

  for(int i = 0; i < params["number"]; i++) {
    // Process data
    plugin.get_output(output);
    cout << "Clock: " << output << endl;
  }

  return 0;
}
