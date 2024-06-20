/*
  ____           _                 
 |  _ \ __ _  __| | ___  _ __ ___  
 | |_) / _` |/ _` |/ _ \| '_ ` _ \ 
 |  _ < (_| | (_| | (_) | | | | | |
 |_| \_\__,_|\__,_|\___/|_| |_| |_|
                                   
Generate random numbers
*/

#include "../source.hpp"
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <Eigen/Dense>
#define STATS_ENABLE_EIGEN_WRAPPERS
#include <stats.hpp>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "random"
#endif

using namespace std;
using json = nlohmann::json;

// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class Random : public Source<json> {
public:
  string kind() override { return PLUGIN_NAME; }

  return_type get_output(json &out, std::vector<unsigned char> *blob = nullptr) override {
    Eigen::MatrixXd data = stats::rnorm<Eigen::MatrixXd>(1,_number_of_elements,42.0, 1.0);
    vector<double> data_vector{data.data(), data.data() + data.size()};
    out.clear();
    out["result"] = data_vector;
    out["agent_id"] = _agent_id;
    out["_agent_id"] = _agent_id;
    return return_type::success;
  }

  void set_params(void const *params) override { 
    Source::set_params(params);
    try {
      _number_of_elements = _params["number_of_elements"];
    } catch (json::exception &e) {
      _number_of_elements = 10;
    }
  }

  map<string, string> info() override {
    return {};
  };

private:
  int _number_of_elements;
  json _data;
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
INSTALL_SOURCE_DRIVER(Random, json)

/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  Random random;
  json output;
  json params = {{"number_of_elements", 10}};

  // Set parameters
  random.set_params(&params);
  
  // Process data
  random.get_output(output);

  // Produce output
  std::cout << "Random data: " << output << std::endl;

  return 0;
}
