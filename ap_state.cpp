#include "ap_state.h"

#include <hkutil/string.h>

#include <apuuid.hpp>
#include <chrono>
#include <exception>
#include <list>
#include <thread>

#include "game_data.h"

constexpr int AP_MAJOR = 0;
constexpr int AP_MINOR = 4;
constexpr int AP_REVISION = 0;

constexpr int ITEM_HANDLING = 7;  // <- all

APState::APState() {
  std::thread([this]() {
    for (;;) {
      {
        std::lock_guard client_guard(client_mutex_);
        if (apclient_) {
          apclient_->poll();
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

    if (apclient_) {
      apclient_->reset();
    }

    apclient_ = std::make_unique<APClient>(ap_get_uuid(""), "Lingo", server);
  }

  inventory_.clear();
  checked_locations_.clear();

  bool connected = false;
  bool has_connection_result = false;

  apclient_->set_room_info_handler([&]() {
    tracker_frame_->SetStatusMessage(
        "Connected to Archipelago server. Authenticating...");

    apclient_->ConnectSlot(player, password, ITEM_HANDLING, {"Tracker"},
                           {AP_MAJOR, AP_MINOR, AP_REVISION});
  });

  apclient_->set_location_checked_handler(
      [&](const std::list<int64_t>& locations) {
        for (const int64_t location_id : locations) {
          checked_locations_.insert(location_id);
          std::cout << "Location: " << location_id << std::endl;
        }

        RefreshTracker();
      });

  apclient_->set_slot_disconnected_handler([&]() {
    tracker_frame_->SetStatusMessage("Disconnected from Archipelago.");
  });

  apclient_->set_socket_disconnected_handler([&]() {
    tracker_frame_->SetStatusMessage("Disconnected from Archipelago.");
  });

  apclient_->set_items_received_handler(
      [&](const std::list<APClient::NetworkItem>& items) {
        for (const APClient::NetworkItem& item : items) {
          // TODO: Progressive items.

          inventory_.insert(item.item);
          std::cout << "Item: " << item.item << std::endl;
        }

        RefreshTracker();
      });

  apclient_->set_slot_connected_handler([&](const nlohmann::json& slot_data) {
    tracker_frame_->SetStatusMessage("Connected to Archipelago!");

    connected = true;
    has_connection_result = true;
  });

  apclient_->set_slot_refused_handler(
      [&](const std::list<std::string>& errors) {
        connected = false;
        has_connection_result = true;

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
  while (!has_connection_result) {
    if (interval == 0) {
      connected = false;
      has_connection_result = true;

      tracker_frame_->SetStatusMessage("Disconnected from Archipelago.");

      wxMessageBox("Timeout while connecting to Archipelago server.",
                   "Connection failed", wxOK | wxICON_ERROR);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    interval--;
  }

  if (connected) {
    for (const MapArea& map_area : GetGameData().GetMapAreas()) {
      for (int section_id = 0; section_id < map_area.locations.size();
           section_id++) {
        const Location& location = map_area.locations.at(section_id);

        int64_t ap_id = apclient_->get_location_id(location.ap_location_name);
        if (ap_id == APClient::INVALID_NAME_ID) {
          std::cout << "Could not find AP location ID for "
                    << location.ap_location_name << std::endl;
        } else {
          ap_id_by_location_id_[{map_area.id, section_id}] = ap_id;
        }
      }
    }

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

void APState::RefreshTracker() { tracker_frame_->UpdateIndicators(); }

APState& GetAPState() {
  static APState* instance = new APState();
  return *instance;
}
