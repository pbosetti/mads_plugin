/*
  ____  _____ ____ _____                     
 |  _ \| ____/ ___|_   _|_      ____ _ _   _ 
 | |_) |  _| \___ \ | | \ \ /\ / / _` | | | |
 |  _ <| |___ ___) || |  \ V  V / (_| | |_| |
 |_| \_\_____|____/ |_|   \_/\_/ \__,_|\__, |
                                       |___/ 
A RESTful Gateway
*/

#include "../source.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
#include <sstream>
#include <restclient-cpp/restclient.h>
#include <restclient-cpp/connection.h>
#include <fstream>
#include <chrono>
#include <thread>


#ifndef PLUGIN_NAME
#define PLUGIN_NAME "RESTway"
#endif

#define API_PATH "/amw4analysis/job"

using namespace std;
using namespace RestClient;
using json = nlohmann::json;

// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class RESTway : public Source<json> {

public: 
  ~RESTway() {
    RestClient::disable();
    delete _conn;
  }

private:
  void conn_accept_json() {
    RestClient::HeaderFields headers;
    headers["Accept"] = "application/json";
    _conn->SetHeaders(headers);
  }

  void conn_accept_binary() {
    RestClient::HeaderFields headers;
    headers["Accept"] = "application/octet-stream";
    _conn->SetHeaders(headers);
  }

  void setup() {
    RestClient::init();
    _conn = new RestClient::Connection(_params["url"]);
    _conn->SetTimeout(5);
    _conn->SetUserAgent("mads/restway");

    // enable following of redirects (default is off)
    _conn->FollowRedirects(true);
    // and limit the number of redirects (default is -1, unlimited)
    _conn->FollowRedirects(true, 3);

    // set headers
    conn_accept_json();

    // Set TSL options and certificates
    _conn->SetVerifyPeer(false);
    _conn->SetVerifyHost(false);
    _conn->SetCertPath("./file.crt.pem");
    _conn->SetCertType("PEM");
    _conn->SetKeyPath("./file.key.pem");

    _initialized = true;
  }

  string get_jobId(string jobName) {
    json result;
    string path = API_PATH;
    cout << "=> GET: " << path << endl;
    conn_accept_json();
    _response = _conn->get(path);
    if (_response.code != 200) {
      throw runtime_error("REST Error getting job list");
    }
    result = json::parse(_response.body);
    for (auto &r : result) {
      if (r["name"] == jobName) {
        return r["id"];
      }
    }
    throw runtime_error("Job not found");
  }

  string get_jobRunId(string jobId) {
    json result;
    string path = API_PATH;
    path += jobId;
    path += "/run/all";
    cout << "=> GET: " << path << endl;
    conn_accept_json();
    _response = _conn->get(path);
    if (_response.code != 200) {
      throw runtime_error("REST Error getting jobRun list");
    }
    result = json::parse(_response.body);
    return result[0]["jobRunId"];
  }

  string get_fileName(string jobId, string jobRunId) {
    json result;
    string path = API_PATH;
    path += jobId;
    path += "/run/";
    path += jobRunId;
    path += "/file/all";
    cout << "=> GET: " << path << endl;
    conn_accept_json();
    _response = _conn->get(path);
    if (_response.code != 200) {
      throw runtime_error("REST Error getting zip files");
    }
    result = json::parse(_response.body);
    return result[0]["fileName"];
  }

  void save_fileContent(string jobId, string jobRunId, string fileName) {
    string path = API_PATH;
    path += jobId;
    path += "/run/";
    path += jobRunId;
    path += "/file?fileName=";
    path += fileName;
    cout << "=> GET: " << path << endl;
    conn_accept_binary();
    _response = _conn->get(path);
    if (_response.code != 200) {
      throw runtime_error("REST Error getting file content");
    }
    ofstream file(fileName, ios::out | ios::binary);
    file.write(_response.body.c_str(), _response.body.size());
    file.close();
  }

  void delete_jobRun(string jobId, string jobRunId) {
    string path = API_PATH;
    path += jobId;
    path += "/run/";
    path += jobRunId;
    cout << "=> DELETE: " << path << endl;
    _response = _conn->del(path);
    if (_response.code != 200) {
      throw runtime_error("REST Error deleting jobRun");
    }
    return;
  }

public:

  string kind() override { return PLUGIN_NAME; }

  return_type get_output(json *out, std::vector<unsigned char> *blob = nullptr) override {
    return_type rc = return_type::success;
    string path = API_PATH;
    string jobId, jobRunId, fileName;
    json result;
    if (!_initialized) {
      setup();
    }

    try {
      jobId = get_jobId(_params["jobId"][0]);
      jobRunId = get_jobRunId(jobId);
      fileName = get_fileName(jobId, jobRunId);
      save_fileContent(jobId, jobRunId, fileName);
      delete_jobRun(jobId, jobRunId);
    } catch (runtime_error &e) {
      cerr << "Error: " << e.what() << endl;
      rc = return_type::error;
      goto exit;
    }

    return rc;

    // path += "/description?jobId=";
    // path += _params["jobId"][0];
    // _response = _conn->get(path);
    // cout << "RESTway: code " << _response.code << ", " << _response.body << endl;
    // (*out)["code"] = _response.code;
    // try {
    //   result = json::parse(_response.body);
    // } catch (json::parse_error &e) {
    //   cerr << "Error parsing REST body as JSON: " << e.what() << endl;
    //   cerr << "when trying to parse: \"" << _response.body << "\"" << endl;
    //   rc = return_type::error;
    //   goto exit;
    // }
    // cout << "*** " << result[0]["fileName"] << " " << result[0]["fileSize"] << " bytes" << endl;

    // path = API_PATH;

    // out->clear();
    // (*out)["url"] = _params["url"];
    // (*out)["path"] = path;
    // (*out)["headers"] = _response.headers;

    // if (_response.code != 200) {
    //   this_thread::sleep_for(chrono::milliseconds(_params["delay"]));
    //   goto exit;
    // }

exit:
    this_thread::sleep_for(chrono::milliseconds(_params["delay"]));
    return rc;
  }

  void set_params(void *params) override { 
    _params["url"] = string("https://192.168.1.55:5444");
    _params["jobId"] = {"MADS"};
    _params["description"] = "RESTway, a RESTful gateway";
    _params["page"] = 0;
    _params["size"] = 20;
    _params["id"] = 0;
    _params["delay"] = 1000;
    _params.merge_patch(*(json *)params);
  }

  map<string, string> info() override {
    map<string, string> info;
    info["url"] = _params["url"];
    info["description"] = _params["description"];
    return info;
  };

private:
  json _data, _params;
  Response _response;
  RestClient::Connection *_conn;
  bool _initialized = false;
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
INSTALL_SOURCE_DRIVER(RESTway, json)

/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  RESTway source;
  json output;
  json params;
  if (argc == 1) {
    params["url"] = "https://192.168.1.55:5444";
    params["jobId"] = {"MADS"};
    params["description"] = "RESTway, a RESTful gateway";
    params["page"] = 0;
    params["size"] = 20;
  } else {
    ifstream file(argv[1]);
    try {
      params = json::parse(file);
    } catch (json::parse_error &e) {
      cerr << "Error parsing parameters: " << e.what() << endl;
      return 1;
    }
    file.close();
  }
  source.set_params(&params);
  for (auto &p : source.info()) {
    cout << "_params[" << p.first << "]: " << p.second << endl;
  }

  // Process data
  source.get_output(&output);

  // Produce output
  cout << "REST returns: " << output << endl;

  return 0;
}
