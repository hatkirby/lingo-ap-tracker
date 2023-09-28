#include "ap_state.h"

#define HAS_STD_FILESYSTEM
#define _WEBSOCKETPP_CPP11_STRICT_
#pragma comment(lib, "crypt32")

#include <hkutil/string.h>

#include <apclient.hpp>
#include <apuuid.hpp>
#include <chrono>
#include <exception>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <tuple>

#include "game_data.h"
#include "logger.h"
#include "tracker_frame.h"
#include "tracker_state.h"

constexpr int AP_MAJOR = 0;
constexpr int AP_MINOR = 4;
constexpr int AP_REVISION = 0;

constexpr const char* CERT_STORE_PATH = "cacert.pem";
constexpr int ITEM_HANDLING = 7;  // <- all

namespace {

struct APState {
  std::unique_ptr<APClient> apclient;

  bool initialized = false;

  TrackerFrame* tracker_frame = nullptr;

  bool client_active = false;
  std::mutex client_mutex;

  bool connected = false;
  bool has_connection_result = false;

  std::string data_storage_prefix;
  std::list<std::string> tracked_data_storage_keys;

  std::map<int64_t, int> inventory;
  std::set<int64_t> checked_locations;
  std::map<std::string, bool> data_storage;

  DoorShuffleMode door_shuffle_mode = kNO_DOORS;
  bool color_shuffle = false;
  bool painting_shuffle = false;
  int mastery_requirement = 21;
  int level_2_requirement = 223;
  LocationChecks location_checks = kNORMAL_LOCATIONS;
  VictoryCondition victory_condition = kTHE_END;
  bool early_color_hallways = false;

  std::map<std::string, std::string> painting_mapping;

  void Connect(std::string server, std::string player, std::string password) {
    if (!initialized) {
      TrackerLog("Initializing APState...");

      std::thread([this]() {
        for (;;) {
          {
            std::lock_guard client_guard(client_mutex);
            if (apclient) {
              apclient->poll();
            }
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }).detach();

      for (int panel_id : GD_GetAchievementPanels()) {
        tracked_data_storage_keys.push_back(
            "Achievement|" + GD_GetPanel(panel_id).achievement_name);
      }

      initialized = true;
    }

    tracker_frame->SetStatusMessage("Connecting to Archipelago server....");
    TrackerLog("Connecting to Archipelago server (" + server + ")...");

    {
      TrackerLog("Destroying old AP client...");

      std::lock_guard client_guard(client_mutex);

      if (apclient) {
        DestroyClient();
      }

      std::string cert_store = "";
      if (std::filesystem::exists(CERT_STORE_PATH)) {
        cert_store = CERT_STORE_PATH;
      }

      apclient = std::make_unique<APClient>(ap_get_uuid(""), "Lingo", server,
                                            cert_store);
    }

    inventory.clear();
    checked_locations.clear();
    data_storage.clear();
    door_shuffle_mode = kNO_DOORS;
    color_shuffle = false;
    painting_shuffle = false;
    painting_mapping.clear();
    mastery_requirement = 21;
    level_2_requirement = 223;
    location_checks = kNORMAL_LOCATIONS;
    victory_condition = kTHE_END;
    early_color_hallways = false;

    connected = false;
    has_connection_result = false;

    apclient->set_room_info_handler([this, player, password]() {
      inventory.clear();

      TrackerLog("Connected to Archipelago server. Authenticating as " +
                 player +
                 (password.empty() ? " without password"
                                   : " with password " + password));
      tracker_frame->SetStatusMessage(
          "Connected to Archipelago server. Authenticating...");

      apclient->ConnectSlot(player, password, ITEM_HANDLING, {"Tracker"},
                            {AP_MAJOR, AP_MINOR, AP_REVISION});
    });

    apclient->set_location_checked_handler(
        [this](const std::list<int64_t>& locations) {
          for (const int64_t location_id : locations) {
            checked_locations.insert(location_id);
            TrackerLog("Location: " + std::to_string(location_id));
          }

          RefreshTracker();
        });

    apclient->set_slot_disconnected_handler([this]() {
      tracker_frame->SetStatusMessage(
          "Disconnected from Archipelago. Attempting to reconnect...");
      TrackerLog(
          "Slot disconnected from Archipelago. Attempting to reconnect...");
    });

    apclient->set_socket_disconnected_handler([this]() {
      tracker_frame->SetStatusMessage(
          "Disconnected from Archipelago. Attempting to reconnect...");
      TrackerLog(
          "Socket disconnected from Archipelago. Attempting to reconnect...");
    });

    apclient->set_items_received_handler(
        [this](const std::list<APClient::NetworkItem>& items) {
          for (const APClient::NetworkItem& item : items) {
            inventory[item.item]++;
            TrackerLog("Item: " + std::to_string(item.item));
          }

          RefreshTracker();
        });

    apclient->set_retrieved_handler(
        [this](const std::map<std::string, nlohmann::json>& data) {
          for (const auto& [key, value] : data) {
            if (value.is_boolean()) {
              data_storage[key] = value.get<bool>();
              TrackerLog("Data storage " + key + " retrieved as " +
                         (value.get<bool>() ? "true" : "false"));
            }
          }

          RefreshTracker();
        });

    apclient->set_set_reply_handler([this](const std::string& key,
                                           const nlohmann::json& value,
                                           const nlohmann::json&) {
      if (value.is_boolean()) {
        data_storage[key] = value.get<bool>();
        TrackerLog("Data storage " + key + " set to " +
                   (value.get<bool>() ? "true" : "false"));

        RefreshTracker();
      }
    });

    apclient->set_slot_connected_handler([this](
                                             const nlohmann::json& slot_data) {
      tracker_frame->SetStatusMessage("Connected to Archipelago!");
      TrackerLog("Connected to Archipelago!");

      data_storage_prefix =
          "Lingo_" + std::to_string(apclient->get_player_number()) + "_";
      door_shuffle_mode = slot_data["shuffle_doors"].get<DoorShuffleMode>();
      color_shuffle = slot_data["shuffle_colors"].get<int>() == 1;
      painting_shuffle = slot_data["shuffle_paintings"].get<int>() == 1;
      mastery_requirement = slot_data["mastery_achievements"].get<int>();
      level_2_requirement = slot_data["level_2_requirement"].get<int>();
      location_checks = slot_data["location_checks"].get<LocationChecks>();
      victory_condition =
          slot_data["victory_condition"].get<VictoryCondition>();
      early_color_hallways = slot_data.contains("early_color_hallways") &&
                             slot_data["early_color_hallways"].get<int>() == 1;

      if (painting_shuffle && slot_data.contains("painting_entrance_to_exit")) {
        painting_mapping.clear();

        for (const auto& mapping_it :
             slot_data["painting_entrance_to_exit"].items()) {
          painting_mapping[mapping_it.key()] = mapping_it.value();
        }
      }

      connected = true;
      has_connection_result = true;

      RefreshTracker();

      std::list<std::string> corrected_keys;
      for (const std::string& key : tracked_data_storage_keys) {
        corrected_keys.push_back(data_storage_prefix + key);
      }

      apclient->Get(corrected_keys);
      apclient->SetNotify(corrected_keys);
    });

    apclient->set_slot_refused_handler(
        [this](const std::list<std::string>& errors) {
          connected = false;
          has_connection_result = true;

          tracker_frame->SetStatusMessage("Disconnected from Archipelago.");

          std::vector<std::string> error_messages;
          error_messages.push_back("Could not connect to Archipelago.");

          for (const std::string& error : errors) {
            if (error == "InvalidSlot") {
              error_messages.push_back("Invalid player name.");
            } else if (error == "InvalidGame") {
              error_messages.push_back(
                  "The specified player is not playing Lingo.");
            } else if (error == "IncompatibleVersion") {
              error_messages.push_back(
                  "The Archipelago server is not the correct version for this "
                  "client.");
            } else if (error == "InvalidPassword") {
              error_messages.push_back("Incorrect password.");
            } else if (error == "InvalidItemsHandling") {
              error_messages.push_back(
                  "Invalid item handling flag. This is a bug with the tracker. "
                  "Please report it to the lingo-ap-tracker GitHub.");
            } else {
              error_messages.push_back("Unknown error.");
            }
          }

          std::string full_message = hatkirby::implode(error_messages, " ");
          TrackerLog(full_message);

          wxMessageBox(full_message, "Connection failed", wxOK | wxICON_ERROR);
        });

    client_active = true;

    int timeout = 5000;  // 5 seconds
    int interval = 100;
    int remaining_loops = timeout / interval;
    while (!has_connection_result) {
      if (interval == 0) {
        connected = false;
        has_connection_result = true;

        DestroyClient();

        tracker_frame->SetStatusMessage("Disconnected from Archipelago.");

        TrackerLog("Timeout while connecting to Archipelago server.");
        wxMessageBox("Timeout while connecting to Archipelago server.",
                     "Connection failed", wxOK | wxICON_ERROR);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      interval--;
    }

    if (connected) {
      RefreshTracker();
    } else {
      client_active = false;
    }
  }

  bool HasCheckedGameLocation(int location_id) {
    return checked_locations.count(location_id);
  }

  bool HasItem(int item_id, int quantity) {
    return inventory.count(item_id) && inventory.at(item_id) >= quantity;
  }

  bool HasAchievement(const std::string& name) {
    std::string key = data_storage_prefix + "Achievement|" + name;
    return data_storage.count(key) && data_storage.at(key);
  }

  void RefreshTracker() {
    TrackerLog("Refreshing display...");

    RecalculateReachability();
    tracker_frame->UpdateIndicators();
  }

  int64_t GetItemId(const std::string& item_name) {
    int64_t ap_id = apclient->get_item_id(item_name);
    if (ap_id == APClient::INVALID_NAME_ID) {
      TrackerLog("Could not find AP item ID for " + item_name);
    }

    return ap_id;
  }

  void DestroyClient() {
    client_active = false;
    apclient->reset();
    apclient.reset();
  }
};

APState& GetState() {
  static APState* instance = new APState();
  return *instance;
}

}  // namespace

void AP_SetTrackerFrame(TrackerFrame* arg) { GetState().tracker_frame = arg; }

void AP_Connect(std::string server, std::string player, std::string password) {
  GetState().Connect(server, player, password);
}

bool AP_HasCheckedGameLocation(int location_id) {
  return GetState().HasCheckedGameLocation(location_id);
}

bool AP_HasItem(int item_id, int quantity) {
  return GetState().HasItem(item_id, quantity);
}

DoorShuffleMode AP_GetDoorShuffleMode() { return GetState().door_shuffle_mode; }

bool AP_IsColorShuffle() { return GetState().color_shuffle; }

bool AP_IsPaintingShuffle() { return GetState().painting_shuffle; }

const std::map<std::string, std::string> AP_GetPaintingMapping() {
  return GetState().painting_mapping;
}

int AP_GetMasteryRequirement() { return GetState().mastery_requirement; }

int AP_GetLevel2Requirement() { return GetState().level_2_requirement; }

bool AP_IsLocationVisible(int classification) {
  switch (GetState().location_checks) {
    case kNORMAL_LOCATIONS:
      return classification & kLOCATION_NORMAL;
    case kREDUCED_LOCATIONS:
      return classification & kLOCATION_REDUCED;
    case kPANELSANITY:
      return classification & kLOCATION_INSANITY;
    default:
      return false;
  }
}

VictoryCondition AP_GetVictoryCondition() {
  return GetState().victory_condition;
}

bool AP_HasAchievement(const std::string& achievement_name) {
  return GetState().HasAchievement(achievement_name);
}

bool AP_HasEarlyColorHallways() { return GetState().early_color_hallways; }
