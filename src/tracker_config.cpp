#include "tracker_config.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

constexpr const char* CONFIG_FILE_NAME = "config.yaml";

void TrackerConfig::Load() {
  try {
    YAML::Node file = YAML::LoadFile(CONFIG_FILE_NAME);

    ap_server = file["ap_server"].as<std::string>();
    ap_player = file["ap_player"].as<std::string>();
    ap_password = file["ap_password"].as<std::string>();
    asked_to_check_for_updates = file["asked_to_check_for_updates"].as<bool>();
    should_check_for_updates = file["should_check_for_updates"].as<bool>();
  } catch (const std::exception&) {
    // It's fine if the file can't be loaded.
  }
}

void TrackerConfig::Save() {
  YAML::Node output;
  output["ap_server"] = ap_server;
  output["ap_player"] = ap_player;
  output["ap_password"] = ap_password;
  output["asked_to_check_for_updates"] = asked_to_check_for_updates;
  output["should_check_for_updates"] = should_check_for_updates;

  std::ofstream filewriter(CONFIG_FILE_NAME);
  filewriter << output;
}

TrackerConfig& GetTrackerConfig() {
  static TrackerConfig* instance = new TrackerConfig();
  return *instance;
}
