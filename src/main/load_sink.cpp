#include "../sink.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <thread>
#include <vector>
#include <fstream>

using namespace std;

using json = nlohmann::json;
using SinkJ = Sink<json>;
using SinkDriverJ = SinkDriver<json>;

int main(int argc, char *argv[]) {
  pugg::Kernel kernel;
  string json_file = "";
  // add a generic server to the kernel to initilize it
  // kernel.add_server(Sink<>::sink_name(),
  //                   Sink<>::version);
  kernel.add_server<Sink<>>();

  // CLI needs unoe or two plugin paths
  // the first on must have doubles as input and output
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <plugin> [name] [json]" << endl;
    return 1;
  }

  cout << "Loading plugin... ";
  cout.flush();
  // load the plugin
  kernel.load_plugin(argv[1]);

  // find the proper driver in the plugin
  // - if there's only one, load it
  // - if there are more, list them and select the one passed on the CLI
  auto drivers = kernel.get_all_drivers<SinkDriverJ>(SinkJ::server_name());
  SinkDriverJ *driver = nullptr;
  if (drivers.size() == 1) {
    driver = drivers[0];
    cout << "loaded default driver " << driver->name();
    if (argc >= 3) json_file = argv[2]; 
  } else if (drivers.size() > 1) {
    cout << "found multiple drivers:" << endl;
    for (auto &d : drivers) {
      cout << " - " << d->name();
      if (argc >= 3 && d->name() == argv[2]) {
        driver = d;
        cout << " -> selected" << endl;
        if (argc >= 4) json_file = argv[3];
      } else {
        cout << endl;
      }
    }
  }

  // No driver can be loaded
  if (!driver) {
    cout << "\nNo driver to load, exiting" << endl;
    exit(1);
  }

  SinkJ *sink = driver->create();
  // Now we can create an instance of class SinkJ from the driver
  cout << "\nLoaded plugin: " << sink->kind() << endl;

  json in = {{"array", {1, 2, 3, 4}}};
  json params;
  if (argc == 3) {
    ifstream file(argv[2]);
    params = json::parse(file);
  } else {
    params["name"] = "echo test";
  }
  sink->set_params(params);
  for (auto &[k, v]: sink->info()) {
    cout << k << ": " << v << endl;
  }
  sink->load_data(in);
  cout << "Input: " << in << endl;
  delete sink;

  kernel.clear_drivers();
}