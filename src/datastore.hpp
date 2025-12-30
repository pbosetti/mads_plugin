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
  Datastore() : _datastore_path("") {}

  ~Datastore() {
    save();
  }

  void prepare(std::string name) {
    if (name.size() < 5 || name.substr(name.size() - 5) != ".json")
      name += ".json";
    if (_datastore_path.empty())
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

  void prepare(std::string name, std::filesystem::path path) {
    if (!path.has_stem()) 
      throw(std::runtime_error("Wrong path, must be a file"));
    _datastore_path = path;
    prepare(name);
  }

  void prepare(std::string name, std::string path) {
    prepare(name, std::filesystem::path(path));
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