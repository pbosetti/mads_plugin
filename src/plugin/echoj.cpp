/*
  _____     _               _         _             _
 | ____|___| |__   ___     | |  _ __ | |_   _  __ _(_)_ __
 |  _| / __| '_ \ / _ \ _  | | | '_ \| | | | |/ _` | | '_ \
 | |__| (__| | | | (_) | |_| | | |_) | | |_| | (_| | | | | |
 |_____\___|_| |_|\___/ \___/  | .__/|_|\__,_|\__, |_|_| |_|
                               |_|            |___/
It takes a JSON input and returns it as output, enriched with the parameters
*/

#include "../filter.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "echoj"
#endif

using namespace std;
using json = nlohmann::json;


// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class Echo : public Filter<json, json> {
public:
  string kind() override { return PLUGIN_NAME; }
  return_type load_data(json &d) override {
    _data = d;
    return return_type::success;
  }
  return_type process(json *out) override {
    (*out)["data"] = _data;
    (*out)["params"] = _params;
    (*out)["agent_id"] = _agent_id;
    return return_type::success;
  }

  void set_params(void *params) override { 
    Filter::set_params(params);
    _params = *(json *)params; 
  }

  map<string, string> info() override {
    map<string, string> m;
    m["params"] = _params.dump();
    return m;
  };

private:
  json _data, _params;
};



/*
  ____  _             _             _      _                
 |  _ \| |_   _  __ _(_)_ __     __| |_ __(_)_   _____ _ __ 
 | |_) | | | | |/ _` | | '_ \   / _` | '__| \ \ / / _ \ '__|
 |  __/| | |_| | (_| | | | | | | (_| | |  | |\ V /  __/ |   
 |_|   |_|\__,_|\__, |_|_| |_|  \__,_|_|  |_| \_/ \___|_|   
                |___/                                      
This is the plugin driver, it should not need to be modified 
*/
INSTALL_FILTER_DRIVER(Echo, json, json)




/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
#include <sstream>
int main(int argc, char const *argv[]) {
  Echo echo;
  json params = {{"times", 3}};
  json data = {{"array", {1, 2, 3, 4}}};

  // Parse input: with - swutch, read from stdin a single JSON object with
  // "data" and "params" keys
  if (argc > 1 && argv[1] == string("-")) {
    stringstream ss;
    while (cin >> ss.rdbuf()) {}
    json j = json::parse(ss.str());
    data = j["data"];
    params = j["params"];
  } else if (argc > 1) {
    cout << "Usage: " << argv[0] << " [-]" << endl;
    cout << "       use the optional '-' to read input from stdin" << endl;
    return 1;
  }

  // Process data
  echo.set_params(&params);
  echo.load_data(data);
  json result;
  if (echo.process(&result) != return_type::success) {
    cerr << "Error processing data" << endl;
    return 1;
  }

  // Produce output
  cout << "Input: " << data << endl;
  cout << "Output: " << result << endl;

  return 0;
}
