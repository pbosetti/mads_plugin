/*
  ____  _____ ____ _____                     
 |  _ \| ____/ ___|_   _|_      ____ _ _   _ 
 | |_) |  _| \___ \ | | \ \ /\ / / _` | | | |
 |  _ <| |___ ___) || |  \ V  V / (_| | |_| |
 |_| \_\_____|____/ |_|   \_/\_/ \__,_|\__, |
                                       |___/ 
A RESTful Gateway
*/

#include "../source.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <sstream>
#include <restclient-cpp/restclient.h>
#include <fstream>
#include <chrono>
#include <thread>


#ifndef PLUGIN_NAME
#define PLUGIN_NAME "RESTway"
#endif

using namespace std;
using namespace RestClient;
using json = nlohmann::json;

// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class RESTway : public Source<json> {
public:
  string kind() override { return PLUGIN_NAME; }

  return_type get_output(json *out, std::vector<unsigned char> *blob = nullptr) override {
    return_type rc = return_type::error;
    stringstream ss;
    ss << string(_params["url"]) << "?page=" << _params["page"] << "&size=" << _params["size"];
    out->clear();
    (*out)["url"] = ss.str();

    _response = get(ss.str());
    (*out)["code"] = _response.code;
    try {
      (*out)["result"] = json::parse(_response.body);
      rc = return_type::success;
    } catch (json::parse_error &e) {
      cerr << "Error parsing REST body as JSON: " << e.what() << endl;
      cerr << "when trying to parse: \"" << _response.body << "\"" << endl;
      goto exit;
    }
    (*out)["headers"] = _response.headers;

    if (_response.code != 200) {
      this_thread::sleep_for(chrono::milliseconds(_params["delay"]));
      goto exit;
    }

exit:
    this_thread::sleep_for(chrono::milliseconds(_params["delay"]));
    return rc;
  }

  void set_params(void *params) override { 
    _params["url"] = string("http://localhost:5443/amw4analysis/job"); 
    _params["description"] = "RESTway, a RESTful gateway";
    _params["page"] = 0;
    _params["size"] = 20;
    _params["id"] = 0;
    _params["delay"] = 1000;
    _params.merge_patch(*(json *)params);
  }

  map<string, string> info() override {
    map<string, string> info;
    info["url"] = _params["url"];
    info["description"] = _params["description"];
    return info;
  };

private:
  json _data, _params;
  Response _response;
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
INSTALL_SOURCE_DRIVER(RESTway, json)

/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  RESTway source;
  json output;
  json params;
  if (argc == 1) {
    params["url"] = "http://localhost:5443/";
    params["description"] = "RESTway, a RESTful gateway";
    params["page"] = 0;
    params["size"] = 20;
  } else {
    ifstream file(argv[1]);
    try {
      params = json::parse(file);
    } catch (json::parse_error &e) {
      cerr << "Error parsing parameters: " << e.what() << endl;
      return 1;
    }
    file.close();
  }
  source.set_params(&params);
  for (auto &p : source.info()) {
    cout << p.first << ": " << p.second << endl;
  }

  // Process data
  source.get_output(&output);

  // Produce output
  cout << "REST returns: " << output << endl;

  return 0;
}