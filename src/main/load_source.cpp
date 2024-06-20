#include "../source.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <thread>
#include <vector>
#include <fstream>

using namespace std;

using json = nlohmann::json;
using SourceJ = Source<json>;
using SourceDriverJ = SourceDriver<json>;

int main(int argc, char *argv[]) {
  pugg::Kernel kernel;
  string json_file = "";
  // add a generic server to the kernel to initilize it
  // kernel.add_server(Filter<>::filter_name(),
  //                   Filter<>::version);
  kernel.add_server<Source<>>();

  // CLI needs une or two plugin paths
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
  auto drivers = kernel.get_all_drivers<SourceDriverJ>(SourceJ::server_name());
  SourceDriverJ *driver = nullptr;
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

  SourceJ *source = driver->create();
  // Now we can create an instance of class SourceJ from the driver
  cout << "\nLoaded plugin: " << source->kind() << endl;

  json params, out;
  if (argc == 3) {
    ifstream file(argv[2]);
    params = json::parse(file);
    file.close();
  } else {
    params["name"] = "plugin test";
  }
  source->set_params(&params);
  for (auto &p: source->info()) {
    cout << p.first << ": " << p.second << endl;
  }
  source->get_output(out);
  cout << "Output: " << out << endl;
  delete source;

  kernel.clear_drivers();
}