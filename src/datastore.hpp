/*
  ____        _            _                 
 |  _ \  __ _| |_ __ _ ___| |_ ___  _ __ ___ 
 | | | |/ _` | __/ _` / __| __/ _ \| '__/ _ \
 | |_| | (_| | || (_| \__ \ || (_) | | |  __/
 |____/ \__,_|\__\__,_|___/\__\___/|_|  \___|
                                             
 Datastore class, implementing persistency on a JSON file
*/

#ifndef DATASTORE_HPP
#define DATASTORE_HPP

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

class Datastore {
public:
  Datastore() {}

  ~Datastore() {
    save();
  }

  void prepare(std::string name) {
    if (name.size() < 5 || name.substr(name.size() - 5) != ".json")
      name += ".json";
    _datastore_path = std::filesystem::temp_directory_path() / "mads" / name;
    if (!std::filesystem::exists(_datastore_path.parent_path())) {
      std::filesystem::create_directories(_datastore_path.parent_path());
    }
    if (std::filesystem::exists(_datastore_path)) {
      std::ifstream ifs(_datastore_path);
      if (ifs.is_open()) {
        try {
          ifs >> _data;
        } catch (...) {
          _data = nlohmann::json{};
        }
        ifs.close();
      }
    } else {
      _data = nlohmann::json{};
    }
  }

  nlohmann::json & operator[](const std::string &key) {
    return _data[key];
  }

  nlohmann::json & data() {
    return _data;
  }
  
  void save() const {
    std::ofstream file(_datastore_path);
    if (file.is_open()) {
      file << _data.dump(2);
    }
  }

  std::string path() const {
    return _datastore_path.string();
  }
  
private:
  std::filesystem::path _datastore_path;
  nlohmann::json _data;
};

#endif // DATASTORE_HPP