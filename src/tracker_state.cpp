#include "tracker_state.h"

#include <list>
#include <map>
#include <set>
#include <tuple>
#include <sstream>

#include "ap_state.h"
#include "game_data.h"
#include "logger.h"

namespace {

struct TrackerState {
  std::map<std::tuple<int, int>, bool> reachability;
};

TrackerState& GetState() {
  static TrackerState* instance = new TrackerState();
  return *instance;
}

bool IsDoorReachable_Helper(int door_id, const std::set<int>& reachable_rooms);

bool IsPanelReachable_Helper(int panel_id,
                             const std::set<int>& reachable_rooms) {
  const Panel& panel_obj = GD_GetPanel(panel_id);

  if (!reachable_rooms.count(panel_obj.room)) {
    return false;
  }

  if (panel_obj.name == "THE MASTER") {
    int achievements_accessible = 0;

    for (int achieve_id : GD_GetAchievementPanels()) {
      if (IsPanelReachable_Helper(achieve_id, reachable_rooms)) {
        achievements_accessible++;

        if (achievements_accessible >= AP_GetMasteryRequirement()) {
          break;
        }
      }
    }

    return (achievements_accessible >= AP_GetMasteryRequirement());
  }

  if (panel_obj.name == "LEVEL 2" && AP_GetVictoryCondition() == kLEVEL_2) {
    int counting_panels_accessible = 0;

    for (int reachable_room : reachable_rooms) {
      const Room& room = GD_GetRoom(reachable_room);

      for (int roomed_panel_id : room.panels) {
        const Panel& roomed_panel = GD_GetPanel(roomed_panel_id);

        if (!roomed_panel.non_counting &&
            IsPanelReachable_Helper(roomed_panel_id, reachable_rooms)) {
          counting_panels_accessible++;
        }
      }
    }

    return (counting_panels_accessible >= AP_GetLevel2Requirement());
  }

  for (int room_id : panel_obj.required_rooms) {
    if (!reachable_rooms.count(room_id)) {
      return false;
    }
  }

  for (int door_id : panel_obj.required_doors) {
    if (!IsDoorReachable_Helper(door_id, reachable_rooms)) {
      return false;
    }
  }

  for (int panel_id : panel_obj.required_panels) {
    if (!IsPanelReachable_Helper(panel_id, reachable_rooms)) {
      return false;
    }
  }

  if (AP_IsColorShuffle()) {
    for (LingoColor color : panel_obj.colors) {
      if (!AP_HasColorItem(color)) {
        return false;
      }
    }
  }

  return true;
}

bool IsDoorReachable_Helper(int door_id, const std::set<int>& reachable_rooms) {
  const Door& door_obj = GD_GetDoor(door_id);

  if (AP_GetDoorShuffleMode() == kNO_DOORS || door_obj.skip_item) {
    if (!reachable_rooms.count(door_obj.room)) {
      return false;
    }

    for (int panel_id : door_obj.panels) {
      if (!IsPanelReachable_Helper(panel_id, reachable_rooms)) {
        return false;
      }
    }

    return true;
  } else if (AP_GetDoorShuffleMode() == kSIMPLE_DOORS &&
             !door_obj.group_name.empty()) {
    return AP_HasItem(door_obj.group_name);
  } else {
    bool has_item = AP_HasItem(door_obj.item_name);

    if (!has_item) {
      for (const ProgressiveRequirement& prog_req : door_obj.progressives) {
        if (AP_HasItem(prog_req.item_name, prog_req.quantity)) {
          has_item = true;
          break;
        }
      }
    }

    return has_item;
  }
}

}  // namespace

void RecalculateReachability() {
  GetState().reachability.clear();

  std::set<int> reachable_rooms;

  std::list<Exit> flood_boundary;
  flood_boundary.push_back({.destination_room = GD_GetRoomByName("Menu")});

  bool reachable_changed = true;
  while (reachable_changed) {
    reachable_changed = false;

    std::list<Exit> new_boundary;
    for (const Exit& room_exit : flood_boundary) {
      if (reachable_rooms.count(room_exit.destination_room)) {
        continue;
      }

      bool valid_transition = false;
      if (room_exit.door.has_value()) {
        if (IsDoorReachable_Helper(*room_exit.door, reachable_rooms)) {
          valid_transition = true;
        } else {
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
      }
    }

    flood_boundary = new_boundary;
  }

  for (const MapArea& map_area : GD_GetMapAreas()) {
    for (int section_id = 0; section_id < map_area.locations.size();
         section_id++) {
      const Location& location_section = map_area.locations.at(section_id);
      bool reachable = reachable_rooms.count(location_section.room);
      if (reachable) {
        for (int panel_id : location_section.panels) {
          reachable &= IsPanelReachable_Helper(panel_id, reachable_rooms);
        }
      }

      GetState().reachability[{map_area.id, section_id}] = reachable;
    }
  }
}

bool IsLocationReachable(int area_id, int section_id) {
  std::tuple<int, int> key = {area_id, section_id};

  if (GetState().reachability.count(key)) {
    return GetState().reachability.at(key);
  } else {
    return false;
  }
}
