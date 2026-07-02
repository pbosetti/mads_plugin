/*
  ____            _       _   ____                _           
 / ___|  ___ _ __(_) __ _| | |  _ \ ___  __ _  __| | ___ _ __ 
 \___ \ / _ \ '__| |/ _` | | | |_) / _ \/ _` |/ _` |/ _ \ '__|
  ___) |  __/ |  | | (_| | | |  _ <  __/ (_| | (_| |  __/ |   
 |____/ \___|_|  |_|\__,_|_| |_| \_\___|\__,_|\__,_|\___|_|   
                                                              
Serial reader plugin, reads data from a serial port and outputs it 
as a JSON object
*/

#include "source.hpp"
#include "serialport.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <memory>
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
    if (!_serialPort) {
      if (filesystem::exists(_params["port"].get<string>()) == false) {
        cout << "Error: port " << _params["port"].get<string>() << " does not exist" << endl;
        _error = "Port does not exist";
        return return_type::critical;
      }
      try {
        _serialPort = std::make_unique<SerialPort>(_params["port"].get<string>().c_str(), _params["baudrate"].get<unsigned>());
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
    size_t attempts = 0;
    out.clear();
    if (!_serialPort) {
      _error = "Serial port not initialized, call set_params() first";
      return return_type::critical;
    }
    do {
      line.clear();
      if (_serialPort->readLine(line) < 0) {
        _error = "Error reading from serial port";
        return return_type::error;
      }
      try {
        out = json::parse(line);
        success = true;
      } catch (json::exception &e) {
        if (++attempts >= _max_attempts) {
          _error = "No valid JSON received from serial port";
          return return_type::error;
        }
      }
    } while (success == false);
    if (!_agent_id.empty()) out["agent_id"] = _agent_id;
    return return_type::success;
  }

  void set_params(const json &params) override { 
    Source::set_params(params);
    _params["port"] = "/dev/ttyUSB0";
    _params["baudrate"] = 115200;
    _params.merge_patch(params);
    if (setup() != return_type::success) {
      throw std::runtime_error("Error setting up serial port");
    }
    if (_params.find("cfg_cmd") != _params.end()) {
      _serialPort->write(_params["cfg_cmd"].get<string>().c_str());
      _serialPort->write("\n");
    }
  }

  map<string, string> info() override {
    return {
      {"port", _params["port"].get<string>()},
      {"baudrate", to_string(_params["baudrate"].get<unsigned>())},
      {"cfg_cmd", _params.value("cfg_cmd", "")}
    };
  };

private:
  json _data, _params;
  std::unique_ptr<SerialPort> _serialPort;
  static const size_t _max_attempts = 100;
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
MADS_REGISTER_PLUGINS(SerialReader)

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
  sr.set_params(params);

  for (int i = 0; i < 10; i++) {
    sr.get_output(output);
    cout << "message #" << i << ": " << output << endl;
  }

  return 0;
}
