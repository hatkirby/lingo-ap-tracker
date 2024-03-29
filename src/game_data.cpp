#include "game_data.h"

#include <hkutil/string.h>
#include <yaml-cpp/yaml.h>

#include <iostream>
#include <sstream>

#include "logger.h"

namespace {

LingoColor GetColorForString(const std::string &str) {
  if (str == "black") {
    return LingoColor::kBlack;
  } else if (str == "red") {
    return LingoColor::kRed;
  } else if (str == "blue") {
    return LingoColor::kBlue;
  } else if (str == "yellow") {
    return LingoColor::kYellow;
  } else if (str == "orange") {
    return LingoColor::kOrange;
  } else if (str == "green") {
    return LingoColor::kGreen;
  } else if (str == "gray") {
    return LingoColor::kGray;
  } else if (str == "brown") {
    return LingoColor::kBrown;
  } else if (str == "purple") {
    return LingoColor::kPurple;
  } else {
    std::ostringstream errmsg;
    errmsg << "Invalid color: " << str;
    TrackerLog(errmsg.str());

    return LingoColor::kNone;
  }
}

struct GameData {
  std::vector<Room> rooms_;
  std::vector<Door> doors_;
  std::vector<Panel> panels_;
  std::vector<MapArea> map_areas_;

  std::map<std::string, int> room_by_id_;
  std::map<std::string, int> door_by_id_;
  std::map<std::string, int> panel_by_id_;
  std::map<std::string, int> area_by_id_;

  std::vector<int> door_definition_order_;

  std::map<std::string, int> room_by_painting_;

  std::vector<int> achievement_panels_;

  std::map<LingoColor, int> ap_id_by_color_;

  bool loaded_area_data_ = false;
  std::set<std::string> malconfigured_areas_;

  GameData() {
    YAML::Node lingo_config = YAML::LoadFile("assets/LL1.yaml");
    YAML::Node areas_config = YAML::LoadFile("assets/areas.yaml");
    YAML::Node pilgrimage_config = YAML::LoadFile("assets/pilgrimage.yaml");
    YAML::Node ids_config = YAML::LoadFile("assets/ids.yaml");

    auto init_color_id = [this, &ids_config](const std::string &color_name) {
      if (ids_config["special_items"] &&
          ids_config["special_items"][color_name]) {
        std::string input_name = color_name;
        input_name[0] = std::tolower(input_name[0]);
        ap_id_by_color_[GetColorForString(input_name)] =
            ids_config["special_items"][color_name].as<int>();
      } else {
        std::ostringstream errmsg;
        errmsg << "Missing AP item ID for color " << color_name;
        TrackerLog(errmsg.str());
      }
    };

    init_color_id("Black");
    init_color_id("Red");
    init_color_id("Blue");
    init_color_id("Yellow");
    init_color_id("Green");
    init_color_id("Orange");
    init_color_id("Purple");
    init_color_id("Brown");
    init_color_id("Gray");

    rooms_.reserve(lingo_config.size() * 2);

    for (const auto &room_it : lingo_config) {
      int room_id = AddOrGetRoom(room_it.first.as<std::string>());
      Room &room_obj = rooms_[room_id];

      for (const auto &entrance_it : room_it.second["entrances"]) {
        int from_room_id = AddOrGetRoom(entrance_it.first.as<std::string>());
        Room &from_room_obj = rooms_[from_room_id];

        switch (entrance_it.second.Type()) {
          case YAML::NodeType::Scalar: {
            // This is just "true".
            from_room_obj.exits.push_back({.destination_room = room_id});
            break;
          }
          case YAML::NodeType::Map: {
            Exit exit_obj;
            exit_obj.destination_room = room_id;

            if (entrance_it.second["door"]) {
              std::string door_room = room_obj.name;
              if (entrance_it.second["room"]) {
                door_room = entrance_it.second["room"].as<std::string>();
              }
              exit_obj.door = AddOrGetDoor(
                  door_room, entrance_it.second["door"].as<std::string>());
            }

            if (entrance_it.second["painting"]) {
              exit_obj.painting = entrance_it.second["painting"].as<bool>();
            }

            from_room_obj.exits.push_back(exit_obj);
            break;
          }
          case YAML::NodeType::Sequence: {
            for (const auto &option : entrance_it.second) {
              Exit exit_obj;
              exit_obj.destination_room = room_id;

              std::string door_room = room_obj.name;
              if (option["room"]) {
                door_room = option["room"].as<std::string>();
              }
              exit_obj.door =
                  AddOrGetDoor(door_room, option["door"].as<std::string>());

              if (option["painting"]) {
                exit_obj.painting = option["painting"].as<bool>();
              }

              from_room_obj.exits.push_back(exit_obj);
            }

            break;
          }
          default: {
            // This shouldn't happen.
            std::cout << "Error reading game data: " << entrance_it
                      << std::endl;
            break;
          }
        }
      }

      if (room_it.second["panels"]) {
        for (const auto &panel_it : room_it.second["panels"]) {
          int panel_id =
              AddOrGetPanel(room_obj.name, panel_it.first.as<std::string>());
          Panel &panel_obj = panels_[panel_id];
          room_obj.panels.push_back(panel_id);

          if (panel_it.second["colors"]) {
            if (panel_it.second["colors"].IsScalar()) {
              panel_obj.colors.push_back(GetColorForString(
                  panel_it.second["colors"].as<std::string>()));
            } else {
              for (const auto &color_node : panel_it.second["colors"]) {
                panel_obj.colors.push_back(
                    GetColorForString(color_node.as<std::string>()));
              }
            }
          }

          if (panel_it.second["required_room"]) {
            if (panel_it.second["required_room"].IsScalar()) {
              panel_obj.required_rooms.push_back(AddOrGetRoom(
                  panel_it.second["required_room"].as<std::string>()));
            } else {
              for (const auto &rr_node : panel_it.second["required_room"]) {
                panel_obj.required_rooms.push_back(
                    AddOrGetRoom(rr_node.as<std::string>()));
              }
            }
          }

          if (panel_it.second["required_door"]) {
            if (panel_it.second["required_door"].IsMap()) {
              std::string rd_room = room_obj.name;
              if (panel_it.second["required_door"]["room"]) {
                rd_room =
                    panel_it.second["required_door"]["room"].as<std::string>();
              }

              panel_obj.required_doors.push_back(AddOrGetDoor(
                  rd_room,
                  panel_it.second["required_door"]["door"].as<std::string>()));
            } else {
              for (const auto &rr_node : panel_it.second["required_door"]) {
                std::string rd_room = room_obj.name;
                if (rr_node["room"]) {
                  rd_room = rr_node["room"].as<std::string>();
                }

                panel_obj.required_doors.push_back(
                    AddOrGetDoor(rd_room, rr_node["door"].as<std::string>()));
              }
            }
          }

          if (panel_it.second["required_panel"]) {
            if (panel_it.second["required_panel"].IsMap()) {
              std::string rp_room = room_obj.name;
              if (panel_it.second["required_panel"]["room"]) {
                rp_room =
                    panel_it.second["required_panel"]["room"].as<std::string>();
              }

              panel_obj.required_panels.push_back(AddOrGetPanel(
                  rp_room, panel_it.second["required_panel"]["panel"]
                               .as<std::string>()));
            } else {
              for (const auto &rp_node : panel_it.second["required_panel"]) {
                std::string rp_room = room_obj.name;
                if (rp_node["room"]) {
                  rp_room = rp_node["room"].as<std::string>();
                }

                panel_obj.required_panels.push_back(
                    AddOrGetPanel(rp_room, rp_node["panel"].as<std::string>()));
              }
            }
          }

          if (panel_it.second["check"]) {
            panel_obj.check = panel_it.second["check"].as<bool>();
          }

          if (panel_it.second["achievement"]) {
            panel_obj.achievement = true;
            panel_obj.achievement_name =
                panel_it.second["achievement"].as<std::string>();

            achievement_panels_.push_back(panel_id);
          }

          if (panel_it.second["exclude_reduce"]) {
            panel_obj.exclude_reduce =
                panel_it.second["exclude_reduce"].as<bool>();
          }

          if (panel_it.second["non_counting"]) {
            panel_obj.non_counting = panel_it.second["non_counting"].as<bool>();
          }

          if (ids_config["panels"] && ids_config["panels"][room_obj.name] &&
              ids_config["panels"][room_obj.name][panel_obj.name]) {
            panel_obj.ap_location_id =
                ids_config["panels"][room_obj.name][panel_obj.name].as<int>();
          } else {
            std::ostringstream errmsg;
            errmsg << "Missing AP location ID for panel " << room_obj.name
                   << " - " << panel_obj.name;
            TrackerLog(errmsg.str());
          }
        }
      }

      if (room_it.second["doors"]) {
        for (const auto &door_it : room_it.second["doors"]) {
          int door_id =
              AddOrGetDoor(room_obj.name, door_it.first.as<std::string>());
          door_definition_order_.push_back(door_id);
          Door &door_obj = doors_[door_id];

          bool has_external_panels = false;
          std::vector<std::string> panel_names;

          for (const auto &panel_node : door_it.second["panels"]) {
            if (panel_node.IsScalar()) {
              panel_names.push_back(panel_node.as<std::string>());
              door_obj.panels.push_back(
                  AddOrGetPanel(room_obj.name, panel_node.as<std::string>()));
            } else {
              has_external_panels = true;
              panel_names.push_back(panel_node["panel"].as<std::string>());
              door_obj.panels.push_back(
                  AddOrGetPanel(panel_node["room"].as<std::string>(),
                                panel_node["panel"].as<std::string>()));
            }
          }

          if (door_it.second["skip_location"]) {
            door_obj.skip_location = door_it.second["skip_location"].as<bool>();
          }

          if (door_it.second["skip_item"]) {
            door_obj.skip_item = door_it.second["skip_item"].as<bool>();
          }

          if (door_it.second["event"]) {
            door_obj.skip_location = door_it.second["event"].as<bool>();
            door_obj.skip_item = door_it.second["event"].as<bool>();
            door_obj.is_event = door_it.second["event"].as<bool>();
          }

          if (door_it.second["item_name"]) {
            door_obj.item_name = door_it.second["item_name"].as<std::string>();
          } else if (!door_it.second["skip_item"] && !door_it.second["event"]) {
            door_obj.item_name = room_obj.name + " - " + door_obj.name;
          }

          if (!door_it.second["skip_item"] && !door_it.second["event"]) {
            if (ids_config["doors"] && ids_config["doors"][room_obj.name] &&
                ids_config["doors"][room_obj.name][door_obj.name] &&
                ids_config["doors"][room_obj.name][door_obj.name]["item"]) {
              door_obj.ap_item_id =
                  ids_config["doors"][room_obj.name][door_obj.name]["item"]
                      .as<int>();
            } else {
              std::ostringstream errmsg;
              errmsg << "Missing AP item ID for door " << room_obj.name << " - "
                     << door_obj.name;
              TrackerLog(errmsg.str());
            }
          }

          if (door_it.second["group"]) {
            door_obj.group_name = door_it.second["group"].as<std::string>();

            if (ids_config["door_groups"] &&
                ids_config["door_groups"][door_obj.group_name]) {
              door_obj.group_ap_item_id =
                  ids_config["door_groups"][door_obj.group_name].as<int>();
            } else {
              std::ostringstream errmsg;
              errmsg << "Missing AP item ID for door group "
                     << door_obj.group_name;
              TrackerLog(errmsg.str());
            }
          }

          if (door_it.second["location_name"]) {
            door_obj.location_name =
                door_it.second["location_name"].as<std::string>();
          } else if (!door_it.second["skip_location"] &&
                     !door_it.second["event"]) {
            if (has_external_panels) {
              std::ostringstream errmsg;
              errmsg
                  << room_obj.name << " - " << door_obj.name
                  << " has panels from other rooms but does not have an "
                     "explicit "
                     "location name and is not marked skip_location or event";
              TrackerLog(errmsg.str());
            }

            door_obj.location_name =
                room_obj.name + " - " + hatkirby::implode(panel_names, ", ");
          }

          if (!door_it.second["skip_location"] && !door_it.second["event"]) {
            if (ids_config["doors"] && ids_config["doors"][room_obj.name] &&
                ids_config["doors"][room_obj.name][door_obj.name] &&
                ids_config["doors"][room_obj.name][door_obj.name]["location"]) {
              door_obj.ap_location_id =
                  ids_config["doors"][room_obj.name][door_obj.name]["location"]
                      .as<int>();
            } else {
              std::ostringstream errmsg;
              errmsg << "Missing AP location ID for door " << room_obj.name
                     << " - " << door_obj.name;
              TrackerLog(errmsg.str());
            }
          }

          if (door_it.second["include_reduce"]) {
            door_obj.exclude_reduce =
                !door_it.second["include_reduce"].as<bool>();
          }
        }
      }

      if (room_it.second["paintings"]) {
        for (const auto &painting : room_it.second["paintings"]) {
          std::string painting_id = painting["id"].as<std::string>();
          room_by_painting_[painting_id] = room_id;

          if (!painting["exit_only"] || !painting["exit_only"].as<bool>()) {
            PaintingExit painting_exit;
            painting_exit.id = painting_id;

            if (painting["required_door"]) {
              std::string rd_room = room_obj.name;
              if (painting["required_door"]["room"]) {
                rd_room = painting["required_door"]["room"].as<std::string>();
              }

              painting_exit.door = AddOrGetDoor(
                  rd_room, painting["required_door"]["door"].as<std::string>());
            }

            room_obj.paintings.push_back(painting_exit);
          }
        }
      }

      if (room_it.second["progression"]) {
        for (const auto &progression_it : room_it.second["progression"]) {
          std::string progressive_item_name =
              progression_it.first.as<std::string>();

          int progressive_item_id = -1;
          if (ids_config["progression"] &&
              ids_config["progression"][progressive_item_name]) {
            progressive_item_id =
                ids_config["progression"][progressive_item_name].as<int>();
          } else {
            std::ostringstream errmsg;
            errmsg << "Missing AP item ID for progressive item "
                   << progressive_item_name;
            TrackerLog(errmsg.str());
          }

          int index = 1;
          for (const auto &stage : progression_it.second) {
            int door_id = -1;

            if (stage.IsScalar()) {
              door_id = AddOrGetDoor(room_obj.name, stage.as<std::string>());
            } else {
              door_id = AddOrGetDoor(stage["room"].as<std::string>(),
                                     stage["door"].as<std::string>());
            }

            doors_[door_id].progressives.push_back(
                {.item_name = progressive_item_name,
                 .ap_item_id = progressive_item_id,
                 .quantity = index});
            index++;
          }
        }
      }
    }

    map_areas_.reserve(areas_config.size());

    std::map<std::string, int> fold_areas;
    for (const auto &area_it : areas_config) {
      if (area_it.second["map"]) {
        int area_id = AddOrGetArea(area_it.first.as<std::string>());
        MapArea &area_obj = map_areas_[area_id];
        area_obj.map_x = area_it.second["map"][0].as<int>();
        area_obj.map_y = area_it.second["map"][1].as<int>();
      } else if (area_it.second["fold_into"]) {
        fold_areas[area_it.first.as<std::string>()] =
            AddOrGetArea(area_it.second["fold_into"].as<std::string>());
      }
    }

    loaded_area_data_ = true;

    // Only locations for the panels are kept here.
    std::map<std::string, std::tuple<int, int>> locations_by_name;

    for (const Panel &panel : panels_) {
      int room_id = panel.room;
      std::string room_name = rooms_[room_id].name;

      std::string area_name = room_name;
      if (fold_areas.count(room_name)) {
        int fold_area_id = fold_areas[room_name];
        area_name = map_areas_[fold_area_id].name;
      }

      int classification = kLOCATION_INSANITY;
      if (panel.check) {
        classification |= kLOCATION_NORMAL;
        if (!panel.exclude_reduce) {
          classification |= kLOCATION_REDUCED;
        }
      }

      int area_id = AddOrGetArea(area_name);
      MapArea &map_area = map_areas_[area_id];
      // room field should be the original room ID
      map_area.locations.push_back(
          {.name = panel.name,
           .ap_location_name = room_name + " - " + panel.name,
           .ap_location_id = panel.ap_location_id,
           .room = panel.room,
           .panels = {panel.id},
           .classification = classification});
      locations_by_name[map_area.locations.back().ap_location_name] = {
          area_id, map_area.locations.size() - 1};
    }

    for (int door_id : door_definition_order_) {
      const Door &door = doors_.at(door_id);

      if (!door.skip_location) {
        int classification = kLOCATION_NORMAL;
        if (!door.exclude_reduce) {
          classification |= kLOCATION_REDUCED;
        }

        if (locations_by_name.count(door.location_name)) {
          auto [area_id, section_id] = locations_by_name[door.location_name];
          map_areas_[area_id].locations[section_id].classification |=
              classification;
        } else {
          int room_id = door.room;
          std::string area_name = rooms_[room_id].name;
          std::string section_name;

          size_t divider_pos = door.location_name.find(" - ");
          if (divider_pos == std::string::npos) {
            section_name = door.location_name;
          } else {
            area_name = door.location_name.substr(0, divider_pos);
            section_name = door.location_name.substr(divider_pos + 3);
          }

          if (fold_areas.count(area_name)) {
            int fold_area_id = fold_areas[area_name];
            area_name = map_areas_[fold_area_id].name;
          }

          int area_id = AddOrGetArea(area_name);
          MapArea &map_area = map_areas_[area_id];
          // room field should be the original room ID
          map_area.locations.push_back({.name = section_name,
                                        .ap_location_name = door.location_name,
                                        .ap_location_id = door.ap_location_id,
                                        .room = door.room,
                                        .panels = door.panels,
                                        .classification = classification});
        }
      }
    }

    for (MapArea &map_area : map_areas_) {
      for (const Location &location : map_area.locations) {
        map_area.classification |= location.classification;
      }
    }

    // Set up fake pilgrimage.
    int fake_pilgrim_panel_id =
        AddOrGetPanel("Starting Room", "!! Fake Pilgrimage Panel");
    Panel &fake_pilgrim_panel_obj = panels_[fake_pilgrim_panel_id];
    fake_pilgrim_panel_obj.non_counting = true;

    for (const auto &config_node : pilgrimage_config) {
      fake_pilgrim_panel_obj.required_doors.push_back(
          AddOrGetDoor(config_node["room"].as<std::string>(),
                       config_node["door"].as<std::string>()));
    }

    int fake_pilgrim_door_id =
        AddOrGetDoor("Starting Room", "!! Fake Pilgrimage Door");
    Door &fake_pilgrim_door_obj = doors_[fake_pilgrim_door_id];
    fake_pilgrim_door_obj.panels.push_back(fake_pilgrim_panel_id);
    fake_pilgrim_door_obj.skip_location = true;
    fake_pilgrim_door_obj.skip_item = true;
    fake_pilgrim_door_obj.is_event = true;

    int starting_room_id = AddOrGetRoom("Starting Room");
    Room &starting_room_obj = rooms_[starting_room_id];
    starting_room_obj.panels.push_back(fake_pilgrim_panel_id);
    starting_room_obj.exits.push_back(
        Exit{.destination_room = AddOrGetRoom("Pilgrim Antechamber"),
             .door = fake_pilgrim_door_id});

    // Report errors.
    for (const std::string &area : malconfigured_areas_) {
      std::ostringstream errstr;
      errstr << "Area data not found for: " << area;
      TrackerLog(errstr.str());
    }
  }

  int AddOrGetRoom(std::string room) {
    if (!room_by_id_.count(room)) {
      room_by_id_[room] = rooms_.size();
      rooms_.push_back({.name = room});
    }

    return room_by_id_[room];
  }

  int AddOrGetDoor(std::string room, std::string door) {
    std::string full_name = room + " - " + door;

    if (!door_by_id_.count(full_name)) {
      door_by_id_[full_name] = doors_.size();
      doors_.push_back({.room = AddOrGetRoom(room), .name = door});
    }

    return door_by_id_[full_name];
  }

  int AddOrGetPanel(std::string room, std::string panel) {
    std::string full_name = room + " - " + panel;

    if (!panel_by_id_.count(full_name)) {
      int panel_id = panels_.size();
      panel_by_id_[full_name] = panel_id;
      panels_.push_back(
          {.id = panel_id, .room = AddOrGetRoom(room), .name = panel});
    }

    return panel_by_id_[full_name];
  }

  int AddOrGetArea(std::string area) {
    if (!area_by_id_.count(area)) {
      if (loaded_area_data_) {
        malconfigured_areas_.insert(area);
      }

      int area_id = map_areas_.size();
      area_by_id_[area] = area_id;
      map_areas_.push_back({.id = area_id, .name = area});
    }

    return area_by_id_[area];
  }
};

GameData &GetState() {
  static GameData *instance = new GameData();
  return *instance;
}

}  // namespace

const std::vector<MapArea> &GD_GetMapAreas() { return GetState().map_areas_; }

const MapArea &GD_GetMapArea(int id) { return GetState().map_areas_.at(id); }

int GD_GetRoomByName(const std::string &name) {
  return GetState().room_by_id_.at(name);
}

const Room &GD_GetRoom(int room_id) { return GetState().rooms_.at(room_id); }

const std::vector<Door> &GD_GetDoors() { return GetState().doors_; }

const Door &GD_GetDoor(int door_id) { return GetState().doors_.at(door_id); }

const Panel &GD_GetPanel(int panel_id) {
  return GetState().panels_.at(panel_id);
}

int GD_GetRoomForPainting(const std::string &painting_id) {
  return GetState().room_by_painting_.at(painting_id);
}

const std::vector<int> &GD_GetAchievementPanels() {
  return GetState().achievement_panels_;
}

int GD_GetItemIdForColor(LingoColor color) {
  return GetState().ap_id_by_color_.at(color);
}
