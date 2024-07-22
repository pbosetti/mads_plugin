/*
  _____ _ _ _                    _             _       
 |  ___(_) | |_ ___ _ __   _ __ | |_   _  __ _(_)_ __  
 | |_  | | | __/ _ \ '__| | '_ \| | | | |/ _` | | '_ \ 
 |  _| | | | ||  __/ |    | |_) | | |_| | (_| | | | | |
 |_|   |_|_|\__\___|_|    | .__/|_|\__,_|\__, |_|_| |_|
                          |_|            |___/         
A Template for a Filter Plugin
*/
// Mandatory included headers
#include "../filter.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
// other includes as needed here

// Define the name of the plugin
#ifndef PLUGIN_NAME
#define PLUGIN_NAME "CHANGE ME!"
#endif

// Load the namespaces
using namespace std;
using json = nlohmann::json;


// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class PluginClassName : public Filter<json, json> {

public:

  // Typically, no need to change this
  string kind() override { return PLUGIN_NAME; }

  // Implement the actual functionality here
  return_type load_data(json const &input, string topic = "") override {
    // Do something with the input data
    return return_type::success;
  }

  // We calculate the average of the last N values for each key and store it
  // into the output json object
  return_type process(json &out) override {
    out.clear();

    // load the data as necessary and set the fields of the json out variable

    // This sets the agent_id field in the output json object, only when it is
    // not empty
    if (!_agent_id.empty()) out["agent_id"] = _agent_id;
    return return_type::success;
  }
  
  void set_params(void const *params) override {
    // Call the parent class method to set the common parameters 
    // (e.g. agent_id, etc.)
    Filter::set_params(params);

    // provide sensible defaults for the parameters by setting e.g.
    _params["some_field"] = "default_value";
    // more here...

    // then merge the defaults with the actually provided parameters
    // params needs to be cast to json
    _params.merge_patch(*(json *)params);
  }

  // Implement this method if you want to provide additional information
  map<string, string> info() override { 
    // return a map of stringswith additional information about the plugin
    // it is used to print the information about the plugin when it is loaded
    // by the agent
    return {}; 
  };

private:
  // Define the fields that are used to store internal resources
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
INSTALL_FILTER_DRIVER(PluginClassName, json, json);


/*
                  _       
  _ __ ___   __ _(_)_ __  
 | '_ ` _ \ / _` | | '_ \ 
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|
                          
*/

int main(int argc, char const *argv[])
{
  PluginClassName plugin;
  json params;
  json input, output;

  // Set example values to params
  params["test"] = "value";

  // Set the parameters
  plugin.set_params(&params);

  // Set input data
  input["data"] = {
    {"AX", 1},
    {"AY", 2},
    {"AZ", 3}
  };

  // Set input data
  plugin.load_data(input);
  cout << "Input: " << input.dump(2) << endl;

  // Process data
  plugin.process(output);
  cout << "Output: " << output.dump(2) << endl;


  return 0;
}

