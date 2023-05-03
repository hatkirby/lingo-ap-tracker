#include "ap_state.h"

#define HAS_STD_FILESYSTEM
#define _WEBSOCKETPP_CPP11_STRICT_
#pragma comment(lib, "crypt32")

#include <apclient.hpp>
#include <apuuid.hpp>
#include <hkutil/string.h>
#include <chrono>
#include <exception>
#include <list>
#include <thread>

#include "game_data.h"
#include "tracker_state.h"

constexpr int AP_MAJOR = 0;
constexpr int AP_MINOR = 4;
constexpr int AP_REVISION = 0;

constexpr int ITEM_HANDLING = 7;  // <- all

static APClient* apclient = nullptr;

APState::APState() {
  std::thread([this]() {
    for (;;) {
      {
        std::lock_guard client_guard(client_mutex_);
        if (apclient) {
          apclient->poll();
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }).detach();
}

void APState::Connect(std::string server, std::string player,
                      std::string password) {
  tracker_frame_->SetStatusMessage("Connecting to Archipelago server....");

  {
    std::lock_guard client_guard(client_mutex_);

    if (apclient) {
      DestroyClient();
    }

    apclient = new APClient(ap_get_uuid(""), "Lingo", server);
  }

  inventory_.clear();
  checked_locations_.clear();
  door_shuffle_mode_ = kNO_DOORS;
  color_shuffle_ = false;
  painting_shuffle_ = false;
  painting_mapping_.clear();

  connected_ = false;
  has_connection_result_ = false;

  apclient->set_room_info_handler([this, player, password]() {
    tracker_frame_->SetStatusMessage(
        "Connected to Archipelago server. Authenticating...");

    apclient->ConnectSlot(player, password, ITEM_HANDLING, {"Tracker"},
                           {AP_MAJOR, AP_MINOR, AP_REVISION});
  });

  apclient->set_location_checked_handler(
      [this](const std::list<int64_t>& locations) {
        for (const int64_t location_id : locations) {
          checked_locations_.insert(location_id);
          std::cout << "Location: " << location_id << std::endl;
        }

        RefreshTracker();
      });

  apclient->set_slot_disconnected_handler([this]() {
    tracker_frame_->SetStatusMessage("Disconnected from Archipelago. Attempting to reconnect...");
  });

  apclient->set_socket_disconnected_handler([this]() {
    tracker_frame_->SetStatusMessage("Disconnected from Archipelago. Attempting to reconnect...");
  });

  apclient->set_items_received_handler(
      [this](const std::list<APClient::NetworkItem>& items) {
        for (const APClient::NetworkItem& item : items) {
          // TODO: Progressive items.

          inventory_.insert(item.item);
          std::cout << "Item: " << item.item << std::endl;
        }

        RefreshTracker();
      });

  apclient->set_slot_connected_handler([this](const nlohmann::json& slot_data) {
    tracker_frame_->SetStatusMessage("Connected to Archipelago!");

    door_shuffle_mode_ = slot_data["shuffle_doors"].get<DoorShuffleMode>();
    color_shuffle_ = slot_data["shuffle_colors"].get<bool>();
    painting_shuffle_ = slot_data["shuffle_paintings"].get<bool>();

    if (painting_shuffle_ && slot_data.contains("painting_entrance_to_exit")) {
      painting_mapping_.clear();

      for (const auto& mapping_it : slot_data["painting_entrance_to_exit"].items()) {
        painting_mapping_[mapping_it.key()] = mapping_it.value();
      }
    }

    connected_ = true;
    has_connection_result_ = true;
  });

  apclient->set_slot_refused_handler(
      [this](const std::list<std::string>& errors) {
        connected_ = false;
        has_connection_result_ = true;

        tracker_frame_->SetStatusMessage("Disconnected from Archipelago.");

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

        wxMessageBox(full_message, "Connection failed", wxOK | wxICON_ERROR);
      });

  client_active_ = true;

  int timeout = 5000;  // 5 seconds
  int interval = 100;
  int remaining_loops = timeout / interval;
  while (!has_connection_result_) {
    if (interval == 0) {
      connected_ = false;
      has_connection_result_ = true;

      DestroyClient();

      tracker_frame_->SetStatusMessage("Disconnected from Archipelago.");

      wxMessageBox("Timeout while connecting to Archipelago server.",
                   "Connection failed", wxOK | wxICON_ERROR);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    interval--;
  }

  if (connected_) {
    for (const MapArea& map_area : GetGameData().GetMapAreas()) {
      for (int section_id = 0; section_id < map_area.locations.size();
           section_id++) {
        const Location& location = map_area.locations.at(section_id);

        int64_t ap_id = apclient->get_location_id(location.ap_location_name);
        if (ap_id == APClient::INVALID_NAME_ID) {
          std::cout << "Could not find AP location ID for "
                    << location.ap_location_name << std::endl;
        } else {
          ap_id_by_location_id_[{map_area.id, section_id}] = ap_id;
        }
      }
    }

    for (const Door& door : GetGameData().GetDoors()) {
      if (!door.skip_item) {
        ap_id_by_item_name_[door.item_name] = GetItemId(door.item_name);

        if (!door.group_name.empty() &&
            !ap_id_by_item_name_.count(door.group_name)) {
          ap_id_by_item_name_[door.group_name] = GetItemId(door.group_name);
        }
      }
    }

    ap_id_by_color_[LingoColor::kBlack] = GetItemId("Black");
    ap_id_by_color_[LingoColor::kRed] = GetItemId("Red");
    ap_id_by_color_[LingoColor::kBlue] = GetItemId("Blue");
    ap_id_by_color_[LingoColor::kYellow] = GetItemId("Yellow");
    ap_id_by_color_[LingoColor::kPurple] = GetItemId("Purple");
    ap_id_by_color_[LingoColor::kOrange] = GetItemId("Orange");
    ap_id_by_color_[LingoColor::kGreen] = GetItemId("Green");
    ap_id_by_color_[LingoColor::kBrown] = GetItemId("Brown");
    ap_id_by_color_[LingoColor::kGray] = GetItemId("Gray");

    RefreshTracker();
  } else {
    client_active_ = false;
  }
}

bool APState::HasCheckedGameLocation(int area_id, int section_id) const {
  std::tuple<int, int> location_key = {area_id, section_id};

  if (ap_id_by_location_id_.count(location_key)) {
    return checked_locations_.count(ap_id_by_location_id_.at(location_key));
  } else {
    return false;
  }
}

bool APState::HasColorItem(LingoColor color) const {
  if (ap_id_by_color_.count(color)) {
    return inventory_.count(ap_id_by_color_.at(color));
  } else {
    return false;
  }
}

bool APState::HasItem(const std::string& item) const {
  if (ap_id_by_item_name_.count(item)) {
    return inventory_.count(ap_id_by_item_name_.at(item));
  } else {
    return false;
  }
}

void APState::RefreshTracker() {
  GetTrackerState().CalculateState();
  tracker_frame_->UpdateIndicators();
}

int64_t APState::GetItemId(const std::string& item_name) {
  int64_t ap_id = apclient->get_item_id(item_name);
  if (ap_id == APClient::INVALID_NAME_ID) {
    std::cout << "Could not find AP item ID for " << item_name << std::endl;
  }

  return ap_id;
}

void APState::DestroyClient() {
  client_active_ = false;
  apclient->reset();
  delete apclient;
  apclient = nullptr;
}

APState& GetAPState() {
  static APState* instance = new APState();
  return *instance;
}
