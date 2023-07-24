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

  std::map<std::tuple<int, int>, int64_t> ap_id_by_location_id;
  std::map<std::string, int64_t> ap_id_by_item_name;
  std::map<LingoColor, int64_t> ap_id_by_color;
  std::map<int64_t, std::string> progressive_item_by_ap_id;

  DoorShuffleMode door_shuffle_mode = kNO_DOORS;
  bool color_shuffle = false;
  bool painting_shuffle = false;
  int mastery_requirement = 21;
  bool reduce_checks = false;

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
    reduce_checks = false;

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
      color_shuffle = slot_data["shuffle_colors"].get<bool>();
      painting_shuffle = slot_data["shuffle_paintings"].get<bool>();
      mastery_requirement = slot_data["mastery_achievements"].get<int>();
      reduce_checks = (door_shuffle_mode == kNO_DOORS) &&
                      slot_data["reduce_checks"].get<bool>();

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
      for (const MapArea& map_area : GD_GetMapAreas()) {
        for (int section_id = 0; section_id < map_area.locations.size();
             section_id++) {
          const Location& location = map_area.locations.at(section_id);

          int64_t ap_id = apclient->get_location_id(location.ap_location_name);
          if (ap_id == APClient::INVALID_NAME_ID) {
            TrackerLog("Could not find AP location ID for " +
                       location.ap_location_name);
          } else {
            ap_id_by_location_id[{map_area.id, section_id}] = ap_id;
          }
        }
      }

      for (const Door& door : GD_GetDoors()) {
        if (!door.skip_item) {
          ap_id_by_item_name[door.item_name] = GetItemId(door.item_name);

          if (!door.group_name.empty() &&
              !ap_id_by_item_name.count(door.group_name)) {
            ap_id_by_item_name[door.group_name] = GetItemId(door.group_name);
          }

          for (const ProgressiveRequirement& prog_req : door.progressives) {
            ap_id_by_item_name[prog_req.item_name] =
                GetItemId(prog_req.item_name);
          }
        }
      }

      ap_id_by_color[LingoColor::kBlack] = GetItemId("Black");
      ap_id_by_color[LingoColor::kRed] = GetItemId("Red");
      ap_id_by_color[LingoColor::kBlue] = GetItemId("Blue");
      ap_id_by_color[LingoColor::kYellow] = GetItemId("Yellow");
      ap_id_by_color[LingoColor::kPurple] = GetItemId("Purple");
      ap_id_by_color[LingoColor::kOrange] = GetItemId("Orange");
      ap_id_by_color[LingoColor::kGreen] = GetItemId("Green");
      ap_id_by_color[LingoColor::kBrown] = GetItemId("Brown");
      ap_id_by_color[LingoColor::kGray] = GetItemId("Gray");

      RefreshTracker();
    } else {
      client_active = false;
    }
  }

  bool HasCheckedGameLocation(int area_id, int section_id) {
    std::tuple<int, int> location_key = {area_id, section_id};

    if (ap_id_by_location_id.count(location_key)) {
      return checked_locations.count(ap_id_by_location_id.at(location_key));
    } else {
      return false;
    }
  }

  bool HasColorItem(LingoColor color) {
    if (ap_id_by_color.count(color)) {
      return inventory.count(ap_id_by_color.at(color));
    } else {
      return false;
    }
  }

  bool HasItem(const std::string& item, int quantity) {
    if (ap_id_by_item_name.count(item)) {
      int64_t ap_id = ap_id_by_item_name.at(item);
      return inventory.count(ap_id) && inventory.at(ap_id) >= quantity;
    } else {
      return false;
    }
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

bool AP_HasCheckedGameLocation(int area_id, int section_id) {
  return GetState().HasCheckedGameLocation(area_id, section_id);
}

bool AP_HasColorItem(LingoColor color) {
  return GetState().HasColorItem(color);
}

bool AP_HasItem(const std::string& item, int quantity) {
  return GetState().HasItem(item, quantity);
}

DoorShuffleMode AP_GetDoorShuffleMode() { return GetState().door_shuffle_mode; }

bool AP_IsColorShuffle() { return GetState().color_shuffle; }

bool AP_IsPaintingShuffle() { return GetState().painting_shuffle; }

const std::map<std::string, std::string> AP_GetPaintingMapping() {
  return GetState().painting_mapping;
}

int AP_GetMasteryRequirement() { return GetState().mastery_requirement; }

bool AP_IsReduceChecks() { return GetState().reduce_checks; }

bool AP_HasAchievement(const std::string& achievement_name) {
  return GetState().HasAchievement(achievement_name);
}
