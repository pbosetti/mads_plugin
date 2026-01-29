/*
  __  __  ___ _____ _____   ____            _             
 |  \/  |/ _ \_   _|_   _| | __ ) _ __ ___ | | _____ _ __ 
 | |\/| | | | || |   | |   |  _ \| '__/ _ \| |/ / _ \ '__|
 | |  | | |_| || |   | |   | |_) | | | (_) |   <  __/ |   
 |_|  |_|\__\_\|_|   |_|   |____/|_|  \___/|_|\_\___|_|   
                                                          
Acts as a bridge with a MQTT network
*/

#include "../source.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <mosquittopp.h>
#include <sstream>
#include <thread>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "mqtt_bridge"
#endif

using namespace std;
using namespace mosqpp;
using json = nlohmann::json;


class MQTTBridge : public Source<json>, public mosquittopp {
public:
  string kind() override { return PLUGIN_NAME; }

/*
  __  __  ___ _____ _____            _       _           _ 
 |  \/  |/ _ \_   _|_   _|  _ __ ___| | __ _| |_ ___  __| |
 | |\/| | | | || |   | |   | '__/ _ \ |/ _` | __/ _ \/ _` |
 | |  | | |_| || |   | |   | | |  __/ | (_| | ||  __/ (_| |
 |_|  |_|\__\_\|_|   |_|   |_|  \___|_|\__,_|\__\___|\__,_|
                                                           
*/

  return_type setup() {
    return_type status = return_type::success;
    if (_connected) return status;
    string host = _params["broker_host"];
    string topic = _params["topic"];
    int port = _params["broker_port"];

    lib_init();
    reinitialise("MQTT2MADS-bridge", true);
    connect(host.c_str(), port, 60);
    subscribe(NULL, topic.c_str(), 0);

    // Connect to MQTT
    _connected = true;
    return status;
  }

  ~MQTTBridge() {
    disconnect();
    mosqpp::lib_cleanup();
  }

  // void on_connect(int rc) override {
  //   cout << "Connected with code " << rc << endl;
  //   return;
  // }
	
  void on_message(const struct mosquitto_message *message) override {
    _data.clear();
    try {
      _data = json::parse((char *)(message->payload));
      _error = "No error";
    } catch (json::parse_error &e) {
      _error = e.what();
      _data["error"] = "Error parsing invalid JSON received from MQTT";
      _data["reason"] = _error;
      _data["content"] = (char *)(message->payload);
    }
    _topic = message->topic;
    return;
  }


/*
  ____  _    _   _  ____ ___ _   _ 
 |  _ \| |  | | | |/ ___|_ _| \ | |
 | |_) | |  | | | | |  _ | ||  \| |
 |  __/| |__| |_| | |_| || || |\  |
 |_|   |_____\___/ \____|___|_| \_|
                                   
*/
  return_type get_output(json &out, std::vector<unsigned char> *blob = nullptr) override {
    if (setup() != return_type::success) {
      return return_type::critical;
    }
    loop();
    if(_data.is_null() || _data.empty()) {
      _data.clear();
      return return_type::retry;
    }
    out.clear();
    out["payload"] = _data;
    out["topic"] = _topic;
    if (!_agent_id.empty()) out["agent_id"] = _agent_id;
    _data.clear();
    this_thread::sleep_for(chrono::microseconds(500));
    if (_error != "No error") 
      return return_type::error;
    else
      return return_type::success;
  }

  void set_params(const json &params) override { 
    Source::set_params(params);
    _params["broker_host"] = "localhost";
    _params["broker_port"] = 1883;
    _params.merge_patch(params);
  }

  map<string, string> info() override {
    return {
      {"Broker:", _params["broker_host"].get<string>() + ":" + to_string(_params["broker_port"])},
      {"Topic:", _params["topic"]}
    };
  };

private:
  json _data, _params;
  string _topic;
  bool _connected = false;
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
INSTALL_SOURCE_DRIVER(MQTTBridge, json)

/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  MQTTBridge bridge;
  json output, params;
  params["broker_host"] = "localhost";
  params["broker_port"] = 1883;
  params["topic"] = "capture/#";

  // Set parameters
  bridge.set_params(params);

  // Process data
  while (true) {
    if (bridge.get_output(output) == return_type::success) 
      cout << "MQTT: " << output << endl;
  }
  
  return 0;
}
