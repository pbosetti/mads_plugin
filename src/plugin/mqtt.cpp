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
    int port = _params["broker_port"];

    lib_init();
    connect_async(host.c_str(), port, 60);
    subscribe(NULL, "test", 0);

    // Connect to MQTT
    _connected = true;
    return status;
  }

  ~MQTTBridge() {
    disconnect();
    mosqpp::lib_cleanup();
  }

  void on_connect(int rc) override {
    cout << "Connected with code " << rc << endl;
    return;
  }
	
  void on_message(const struct mosquitto_message *message) override {
    cout << "Received message: " << message->payload << endl;
    return;
  }


/*
  ____  _    _   _  ____ ___ _   _ 
 |  _ \| |  | | | |/ ___|_ _| \ | |
 | |_) | |  | | | | |  _ | ||  \| |
 |  __/| |__| |_| | |_| || || |\  |
 |_|   |_____\___/ \____|___|_| \_|
                                   
*/
  return_type get_output(json *out, std::vector<unsigned char> *blob = nullptr) override {
    if (setup() != return_type::success) return return_type::critical;

    loop();
    return return_type::success;
  }

  void set_params(void *params) override { 
    Source::set_params(params);
    _params = *(json *)params; 
  }

  map<string, string> info() override {
    return {};
  };

private:
  json _data, _params;
  bool _connected = false;
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

  // Set parameters
  bridge.set_params(&params);

  // Process data
  bridge.get_output(&output);

  // Produce output
  cout << "MQTT: " << output << endl;

  return 0;
}
