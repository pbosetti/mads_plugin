/*
  ____            _       _   ____                _           
 / ___|  ___ _ __(_) __ _| | |  _ \ ___  __ _  __| | ___ _ __ 
 \___ \ / _ \ '__| |/ _` | | | |_) / _ \/ _` |/ _` |/ _ \ '__|
  ___) |  __/ |  | | (_| | | |  _ <  __/ (_| | (_| |  __/ |   
 |____/ \___|_|  |_|\__,_|_| |_| \_\___|\__,_|\__,_|\___|_|   
                                                              
Serial reader plugin, reads data from a serial port and outputs it 
as a JSON object
*/

#include "../source.hpp"
#include "../serialport.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <sstream>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "serial_reader"
#endif

using namespace std;
using json = nlohmann::json;

// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class SerialReader : public Source<json> {

  return_type setup() {
    if (_serialPort == nullptr) {
      if (filesystem::exists(_params["port"].get<string>()) == false) {
        cout << "Error: port " << _params["port"].get<string>() << " does not exist" << endl;
        _error = "Port does not exist";
        return return_type::critical;
      }
      try {
        _serialPort = new SerialPort(_params["port"].get<string>().c_str(), _params["baudrate"].get<unsigned>());
      } catch (std::exception &e) {
        cout << "Error: " << e.what() << endl;
        _error = e.what();
        return return_type::critical;
      }
    }
    return return_type::success;
  }

public:
  string kind() override { return PLUGIN_NAME; }

  return_type get_output(json &out, std::vector<unsigned char> *blob = nullptr) override {
    string line;
    bool success = false;
    out.clear();
    if (setup() != return_type::success) {
      return return_type::critical;
    }
    do {
      line.clear();
      _serialPort->readLine(line);
      try {
        out = json::parse(line);
        success = true;
      } catch (json::exception &e) {

      }
    } while (success == false);
    out["agent_id"] = _agent_id;
    return return_type::success;
  }

  void set_params(void const *params) override { 
    Source::set_params(params);
    _params["port"] = "/dev/ttyUSB0";
    _params["baudrate"] = 115200;
    _params.merge_patch(*(json *)params);
  }

  map<string, string> info() override {
    return {
      {"port", _params["port"].get<string>()},
      {"baudrate", to_string(_params["baudrate"].get<unsigned>())}
    };
  };

private:
  json _data, _params;
  SerialPort *_serialPort = nullptr;
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
INSTALL_SOURCE_DRIVER(SerialReader, json)

/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  SerialReader sr;
  json output;

  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <port>" << endl;
    return 1;
  }

  // Set parameters
  json params;
  params["port"] = argv[1];
  params["baudrate"] = 115200;
  sr.set_params(&params);

  for (int i = 0; i < 10; i++) {
    sr.get_output(output);
    cout << "message #" << i << ": " << output << endl;
  }

  return 0;
}
