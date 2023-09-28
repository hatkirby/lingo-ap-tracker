#include "tracker_state.h"

#include <list>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <tuple>

#include "ap_state.h"
#include "game_data.h"

namespace {

struct TrackerState {
  std::map<int, bool> reachability;
  std::mutex reachability_mutex;
};

enum Decision { kYes, kNo, kMaybe };

TrackerState& GetState() {
  static TrackerState* instance = new TrackerState();
  return *instance;
}

Decision IsDoorReachable_Helper(int door_id,
                                const std::set<int>& reachable_rooms,
                                const std::set<int>& solveable_panels) {
  const Door& door_obj = GD_GetDoor(door_id);

  if (AP_GetDoorShuffleMode() == kNO_DOORS || door_obj.skip_item) {
    if (!reachable_rooms.count(door_obj.room)) {
      return kMaybe;
    }

    for (int panel_id : door_obj.panels) {
      if (!solveable_panels.count(panel_id)) {
        return kMaybe;
      }
    }

    return kYes;
  } else if (AP_GetDoorShuffleMode() == kSIMPLE_DOORS &&
             !door_obj.group_name.empty()) {
    return AP_HasItem(door_obj.group_ap_item_id) ? kYes : kNo;
  } else {
    bool has_item = AP_HasItem(door_obj.ap_item_id);

    if (!has_item) {
      for (const ProgressiveRequirement& prog_req : door_obj.progressives) {
        if (AP_HasItem(prog_req.ap_item_id, prog_req.quantity)) {
          has_item = true;
          break;
        }
      }
    }

    return has_item ? kYes : kNo;
  }
}

Decision IsPanelReachable_Helper(int panel_id,
                                 const std::set<int>& reachable_rooms,
                                 const std::set<int>& solveable_panels) {
  const Panel& panel_obj = GD_GetPanel(panel_id);

  if (!reachable_rooms.count(panel_obj.room)) {
    return kMaybe;
  }

  if (panel_obj.name == "THE MASTER") {
    int achievements_accessible = 0;

    for (int achieve_id : GD_GetAchievementPanels()) {
      if (solveable_panels.count(achieve_id)) {
        achievements_accessible++;

        if (achievements_accessible >= AP_GetMasteryRequirement()) {
          break;
        }
      }
    }

    return (achievements_accessible >= AP_GetMasteryRequirement()) ? kYes
                                                                   : kMaybe;
  }

  if (panel_obj.name == "ANOTHER TRY" && AP_GetVictoryCondition() == kLEVEL_2) {
    int counting_panels_accessible = 0;

    for (int solved_panel_id : solveable_panels) {
      const Panel& solved_panel = GD_GetPanel(solved_panel_id);

      if (!solved_panel.non_counting) {
        counting_panels_accessible++;
      }
    }

    return (counting_panels_accessible >= AP_GetLevel2Requirement() - 1)
               ? kYes
               : kMaybe;
  }

  for (int room_id : panel_obj.required_rooms) {
    if (!reachable_rooms.count(room_id)) {
      return kMaybe;
    }
  }

  for (int door_id : panel_obj.required_doors) {
    Decision door_reachable =
        IsDoorReachable_Helper(door_id, reachable_rooms, solveable_panels);
    if (door_reachable == kNo) {
      const Door& door_obj = GD_GetDoor(door_id);
      return (door_obj.is_event || AP_GetDoorShuffleMode() == kNO_DOORS)
                 ? kMaybe
                 : kNo;
    } else if (door_reachable == kMaybe) {
      return kMaybe;
    }
  }

  for (int panel_id : panel_obj.required_panels) {
    if (!solveable_panels.count(panel_id)) {
      return kMaybe;
    }
  }

  if (AP_IsColorShuffle()) {
    for (LingoColor color : panel_obj.colors) {
      if (!AP_HasItem(GD_GetItemIdForColor(color))) {
        return kNo;
      }
    }
  }

  return kYes;
}

}  // namespace

void RecalculateReachability() {
  std::set<int> reachable_rooms;
  std::set<int> solveable_panels;

  std::list<int> panel_boundary;
  std::list<Exit> flood_boundary;
  flood_boundary.push_back({.destination_room = GD_GetRoomByName("Menu")});

  if (AP_HasEarlyColorHallways()) {
    flood_boundary.push_back(
        {.destination_room = GD_GetRoomByName("Outside The Undeterred")});
  }

  bool reachable_changed = true;
  while (reachable_changed) {
    reachable_changed = false;

    std::list<int> new_panel_boundary;
    for (int panel_id : panel_boundary) {
      if (solveable_panels.count(panel_id)) {
        continue;
      }

      Decision panel_reachable =
          IsPanelReachable_Helper(panel_id, reachable_rooms, solveable_panels);
      if (panel_reachable == kYes) {
        solveable_panels.insert(panel_id);
        reachable_changed = true;
      } else if (panel_reachable == kMaybe) {
        new_panel_boundary.push_back(panel_id);
      }
    }

    std::list<Exit> new_boundary;
    for (const Exit& room_exit : flood_boundary) {
      if (reachable_rooms.count(room_exit.destination_room)) {
        continue;
      }

      bool valid_transition = false;
      if (room_exit.door.has_value()) {
        Decision door_reachable = IsDoorReachable_Helper(
            *room_exit.door, reachable_rooms, solveable_panels);
        if (door_reachable == kYes) {
          valid_transition = true;
        } else if (door_reachable == kMaybe) {
          new_boundary.push_back(room_exit);
        }
      } else {
        valid_transition = true;
      }

      if (valid_transition) {
        reachable_rooms.insert(room_exit.destination_room);
        reachable_changed = true;

        const Room& room_obj = GD_GetRoom(room_exit.destination_room);
        for (const Exit& out_edge : room_obj.exits) {
          if (!out_edge.painting || !AP_IsPaintingShuffle()) {
            new_boundary.push_back(out_edge);
          }
        }

        if (AP_IsPaintingShuffle()) {
          for (const PaintingExit& out_edge : room_obj.paintings) {
            if (AP_GetPaintingMapping().count(out_edge.id)) {
              Exit painting_exit;
              painting_exit.destination_room = GD_GetRoomForPainting(
                  AP_GetPaintingMapping().at(out_edge.id));
              painting_exit.door = out_edge.door;

              new_boundary.push_back(painting_exit);
            }
          }
        }

        for (int panel_id : room_obj.panels) {
          new_panel_boundary.push_back(panel_id);
        }
      }
    }

    flood_boundary = new_boundary;
    panel_boundary = new_panel_boundary;
  }

  std::map<int, bool> new_reachability;
  for (const MapArea& map_area : GD_GetMapAreas()) {
    for (size_t section_id = 0; section_id < map_area.locations.size();
         section_id++) {
      const Location& location_section = map_area.locations.at(section_id);
      bool reachable = reachable_rooms.count(location_section.room);
      if (reachable) {
        for (int panel_id : location_section.panels) {
          reachable &= (solveable_panels.count(panel_id) == 1);
        }
      }

      new_reachability[location_section.ap_location_id] = reachable;
    }
  }

  {
    std::lock_guard reachability_guard(GetState().reachability_mutex);
    std::swap(GetState().reachability, new_reachability);
  }
}

bool IsLocationReachable(int location_id) {
  std::lock_guard reachability_guard(GetState().reachability_mutex);

  if (GetState().reachability.count(location_id)) {
    return GetState().reachability.at(location_id);
  } else {
    return false;
  }
}
