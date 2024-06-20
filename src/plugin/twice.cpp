/*
  _____          _                  _             _       
 |_   _|_      _(_) ___ ___   _ __ | |_   _  __ _(_)_ __  
   | | \ \ /\ / / |/ __/ _ \ | '_ \| | | | |/ _` | | '_ \ 
   | |  \ V  V /| | (_|  __/ | |_) | | |_| | (_| | | | | |
   |_|   \_/\_/ |_|\___\___| | .__/|_|\__,_|\__, |_|_| |_|
                             |_|            |___/         
*/
#include "../filter.hpp"
#include <pugg/Kernel.h>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "twice"
#endif

using namespace std;
using Vec = std::vector<double>;

struct TwiceParams {
  int times = 2;
};

// The filter class
// We use the defauilt template parameters std::vector<double> for the input and
// output data
class Twice : public Filter<> {
public:
  string kind() override { return PLUGIN_NAME; }

  return_type load_data(Vec const &d) override {
    _data = d;
    return return_type::success;
  }

  return_type process(Vec &out) override {
    out.clear();
    for (auto &d : _data) {
      out.push_back(d * _params.times);
    }
    return return_type::success;
  }
  
  void set_params(void const *params) override {
    Filter::set_params(params);
    _params = *(TwiceParams *)params;
  }

  map<string, string> info() override {

    return {{string("times"), to_string(_params.times)}};
  };

private:
  Vec _data;
  TwiceParams _params;
};


// Enable the class as plugin
INSTALL_FILTER_DRIVER(Twice, Vec, Vec)


/*
                  _       
  _ __ ___   __ _(_)_ __  
 | '_ ` _ \ / _` | | '_ \ 
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|
                          
*/

int main(int argc, char const *argv[])
{
  Twice twice;
  TwiceParams params{3};
  vector<double> data = {1, 2, 3, 4};
  twice.set_params(&params);
  twice.load_data(data);
  vector<double> result(data.size());
  twice.process(&result);
  cout << "Input: " << endl << "{ ";
  for (auto &d : data) {
    cout << d << " ";
  }
  cout << "}" << endl;
  cout << "Output: " << endl << "{ ";
  for (auto &d : result) {
    cout << d << " ";
  }
  cout << "}" << endl;

  return 0;
}

