#include "tracker_state.h"

#include <list>
#include <set>

#include "ap_state.h"
#include "game_data.h"

bool IsDoorReachable_Helper(int door_id, const std::set<int>& reachable_rooms);

bool IsPanelReachable_Helper(int panel_id,
                             const std::set<int>& reachable_rooms) {
  const Panel& panel_obj = GetGameData().GetPanel(panel_id);

  if (!reachable_rooms.count(panel_obj.room)) {
    return false;
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

  if (GetAPState().IsColorShuffle()) {
    for (LingoColor color : panel_obj.colors) {
      if (!GetAPState().HasColorItem(color)) {
        return false;
      }
    }
  }

  return true;
}

bool IsDoorReachable_Helper(int door_id, const std::set<int>& reachable_rooms) {
  const Door& door_obj = GetGameData().GetDoor(door_id);

  if (GetAPState().GetDoorShuffleMode() == kNO_DOORS || door_obj.skip_item) {
    if (!reachable_rooms.count(door_obj.room)) {
      return false;
    }

    for (int panel_id : door_obj.panels) {
      if (!IsPanelReachable_Helper(panel_id, reachable_rooms)) {
        return false;
      }
    }

    return true;
  } else if (GetAPState().GetDoorShuffleMode() == kSIMPLE_DOORS &&
             !door_obj.group_name.empty()) {
    return GetAPState().HasItem(door_obj.group_name);
  } else {
    return GetAPState().HasItem(door_obj.item_name);
  }
}

void TrackerState::CalculateState() {
  reachability_.clear();

  std::set<int> reachable_rooms;

  std::list<Exit> flood_boundary;
  flood_boundary.push_back(
      {.destination_room = GetGameData().GetRoomByName("Menu")});

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
        } else if (GetAPState().GetDoorShuffleMode() == kNO_DOORS) {
          new_boundary.push_back(room_exit);
        }
      } else {
        valid_transition = true;
      }

      if (valid_transition) {
        reachable_rooms.insert(room_exit.destination_room);
        reachable_changed = true;

        const Room& room_obj =
            GetGameData().GetRoom(room_exit.destination_room);
        for (const Exit& out_edge : room_obj.exits) {
          if (!out_edge.painting || !GetAPState().IsPaintingShuffle()) {
            new_boundary.push_back(out_edge);
          }
        }

        if (GetAPState().IsPaintingShuffle()) {
          for (const PaintingExit& out_edge : room_obj.paintings) {
            if (GetAPState().GetPaintingMapping().count(out_edge.id)) {
              Exit painting_exit;
              painting_exit.destination_room = GetGameData().GetRoomForPainting(
                  GetAPState().GetPaintingMapping().at(out_edge.id));
              painting_exit.door = out_edge.door;

              new_boundary.push_back(painting_exit);
            }
          }
        }
      }
    }

    flood_boundary = new_boundary;
  }

  for (const MapArea& map_area : GetGameData().GetMapAreas()) {
    for (int section_id = 0; section_id < map_area.locations.size();
         section_id++) {
      const Location& location_section = map_area.locations.at(section_id);
      bool reachable = reachable_rooms.count(location_section.room);
      if (reachable) {
        for (int panel_id : location_section.panels) {
          reachable &= IsPanelReachable_Helper(panel_id, reachable_rooms);
        }
      }

      reachability_[{map_area.id, section_id}] = reachable;
    }
  }
}

bool TrackerState::IsLocationReachable(int area_id, int section_id) {
  std::tuple<int, int> key = {area_id, section_id};

  if (reachability_.count(key)) {
    return reachability_.at(key);
  } else {
    return false;
  }
}

TrackerState& GetTrackerState() {
  static TrackerState* instance = new TrackerState();
  return *instance;
}
