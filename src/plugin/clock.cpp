/*
   ____ _            _            _             _
  / ___| | ___   ___| | __  _ __ | |_   _  __ _(_)_ __
 | |   | |/ _ \ / __| |/ / | '_ \| | | | |/ _` | | '_ \
 | |___| | (_) | (__|   <  | |_) | | |_| | (_| | | | | |
  \____|_|\___/ \___|_|\_\ | .__/|_|\__,_|\__, |_|_| |_|
                           |_|            |___/
Produces current time and date
*/

#include "../source.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <sstream>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "clock"
#endif

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class Clock : public Source<json> {
public:
  string kind() override { return PLUGIN_NAME; }

  static string get_ISO8601(const system_clock::time_point &time) {
    time_t tt = system_clock::to_time_t(time);
    tm *tt2 = localtime(&tt);

    // Get milliseconds hack
    auto timeTruncated = system_clock::from_time_t(tt);
    int ms =
        std::chrono::duration_cast<milliseconds>(time - timeTruncated).count();

    return (
               stringstream()
               << put_time(tt2, "%FT%T")               // "2023-03-30T19:49:53"
               << "." << setw(3) << setfill('0') << ms // ".005"
               << put_time(tt2, "%z") // "+0200" (time zone offset, optional)
               )
        .str();
  }

  return_type get_output(json &out,
                         std::vector<unsigned char> *blob = nullptr) override {
    auto now = chrono::system_clock::now();
    out.clear();
    out["time_raw"] = now.time_since_epoch().count();
    out["time"] = get_ISO8601(now);
    out["params"] = _params;
    if (!_agent_id.empty()) out["agent_id"] = _agent_id;
    return return_type::success;
  }

  void set_params(void const *params) override {
    Source::set_params(params);
    _params = *(json *)params;
  }

  map<string, string> info() override { return {}; };

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
Enable the class as plugin
*/
INSTALL_SOURCE_DRIVER(Clock, json)

/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  Clock clock;
  json output;

  // Process data
  clock.get_output(output);

  // Produce output
  cout << "Clock: " << output << endl;

  return 0;
}
