/*
  ____                    _                  _                    
 |  _ \ _   _ _ __  _ __ (_)_ __   __ _     / \__   ____ _        
 | |_) | | | | '_ \| '_ \| | '_ \ / _` |   / _ \ \ / / _` |       
 |  _ <| |_| | | | | | | | | | | | (_| |  / ___ \ V / (_| |       
 |_| \_\\__,_|_| |_|_| |_|_|_| |_|\__, | /_/   \_\_/ \__, |       
                                  |___/              |___/        
*/
#include "../filter.hpp"
#include <pugg/Kernel.h>
#include <nlohmann/json.hpp>
#include <deque>
#include <map>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "running-average"
#endif

using namespace std;
using json = nlohmann::json;


// The filter class
// We use the defauilt template parameters std::vector<double> for the input and
// output data
class RunningAverage : public Filter<json, json> {
public:
  string kind() override { return PLUGIN_NAME; }

  return_type load_data(json const &input) override {
    if (input[_params["field"]].is_object() == false) {
      return return_type::error;
    }
    for (auto &it : input[_params["field"]].items()) {
      _queues[it.key()].push_front(it.value());
      if (_queues[it.key()].size() > _params["capa"]) {
        _queues[it.key()].pop_back();
      }
    }
    return return_type::success;
  }

  return_type process(json &out) override {
    out.clear();
    for (auto &[key, queue] : _queues) {
      double sum = 0;
      for (auto &v : queue) {
        sum += v;
      }
      out[_params["out_field"]][key] = sum / queue.size();
      out["size"] = queue.size();
      out["agent_id"] = _agent_id;
    }
    return return_type::success;
  }
  
  void set_params(void const *params) override {
    Filter::set_params(params);
    _params["capa"] = 10;
    _params["field"] = "data";
    _params["out_field"] = "avg";
    _params.merge_patch(*(json *)params);
  }

  map<string, string> info() override {
    return {
      {"capa", to_string(_params["capa"].get<int>())},
      {"field", _params["field"].get<string>()},
      {"out_field", _params["out_field"].get<string>()}
    };
  };

private:
  map<string, deque<double>> _queues;
};


// Enable the class as plugin
INSTALL_FILTER_DRIVER(RunningAverage, json, json);


/*
                  _       
  _ __ ___   __ _(_)_ __  
 | '_ ` _ \ / _` | | '_ \ 
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|
                          
*/

int main(int argc, char const *argv[])
{
  RunningAverage ra;
  json params{{"queue_size", 3}};
  json output;
  json data{
    {"data", {
      {"AX", 1},
      {"AY", 2},
      {"AZ", 3}
    }}
  };

  ra.set_params(&params);

  cout << "Input: " << data << endl;
  ra.load_data(data);
  ra.process(output);
  cout << "Output: " << output << endl;

  data = {
    {"data", {
      {"AX", 4},
      {"AY", 5},
      {"AZ", 6}
    }}
  };
  cout << "Input: " << data << endl;
  ra.load_data(data);
  ra.process(output);
  cout << "Output: " << output << endl;

  data = {
    {"data", {
      {"AX", 7},
      {"AY", 8},
      {"AZ", 9}
    }}
  };
  cout << "Input: " << data << endl;
  ra.load_data(data);
  ra.process(output);
  cout << "Output: " << output << endl;

  data = {
    {"data", {
      {"AX", 10},
      {"AY", 11},
      {"AZ", 12}
    }}
  };
  cout << "Input: " << data.dump(2) << endl;
  ra.load_data(data);
  ra.process(output);
  cout << "Output: " << output.dump(2) << endl;


  return 0;
}

