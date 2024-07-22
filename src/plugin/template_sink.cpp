/*
  ____  _       _            _             _       
 / ___|(_)_ __ | | __  _ __ | |_   _  __ _(_)_ __  
 \___ \| | '_ \| |/ / | '_ \| | | | |/ _` | | '_ \ 
  ___) | | | | |   <  | |_) | | |_| | (_| | | | | |
 |____/|_|_| |_|_|\_\ | .__/|_|\__,_|\__, |_|_| |_|
                      |_|            |___/         
A Template for a Sink Plugin
*/

// Mandatory included headers
#include "../sink.hpp"
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
class PluginClassName : public Sink<json> {

public:

  // Typically, no need to change this
  string kind() override { return PLUGIN_NAME; }

  // Implement the actual functionality here
  return_type load_data(json const &input, string topic = "") override {
    // Do something with the input data
    return return_type::success;
  }

  void set_params(void const *params) override { 
    // Call the parent class method to set the common parameters 
    // (e.g. agent_id, etc.)
    Sink::set_params(params);

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
INSTALL_SINK_DRIVER(PluginClassName, json)


/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  PluginClassName plugin;
  json input, params;
  // Set example values to params
  params["test"] = "value";

  // Set the parameters
  plugin.set_params(&params);

  // Process data
  plugin.load_data(input);

  return 0;
}
