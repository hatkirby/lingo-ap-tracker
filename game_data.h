#ifndef GAME_DATA_H_9C42AC51
#define GAME_DATA_H_9C42AC51

#include <map>
#include <optional>
#include <string>
#include <vector>

enum class LingoColor {
  kNone,
  kBlack,
  kRed,
  kBlue,
  kYellow,
  kGreen,
  kOrange,
  kPurple,
  kBrown,
  kGray
};

struct Panel {
  int id;
  int room;
  std::string name;
  std::vector<LingoColor> colors;
  std::vector<int> required_rooms;
  std::vector<int> required_doors;
  bool check = false;
  bool exclude_reduce = false;
  bool achievement = false;
};

struct ProgressiveRequirement {
  std::string item_name;
  int quantity = 0;
};

struct Door {
  int room;
  std::string name;
  std::string location_name;
  std::string item_name;
  std::string group_name;
  bool skip_location = false;
  bool skip_item = false;
  std::vector<int> panels;
  bool exclude_reduce = true;
  std::vector<ProgressiveRequirement> progressives;
};

struct Exit {
  int destination_room;
  std::optional<int> door;
  bool painting = false;
};

struct PaintingExit {
  std::string id;
  std::optional<int> door;
};

struct Room {
  std::string name;
  std::vector<Exit> exits;
  std::vector<PaintingExit> paintings;
};

struct Location {
  std::string name;
  std::string ap_location_name;
  int room;
  std::vector<int> panels;
};

struct MapArea {
  int id;
  std::string name;
  std::vector<Location> locations;
  int map_x;
  int map_y;
};

class GameData {
 public:
  GameData();

  const std::vector<MapArea>& GetMapAreas() const { return map_areas_; }

  const MapArea& GetMapArea(int id) const { return map_areas_.at(id); }

  int GetRoomByName(const std::string& name) const {
    return room_by_id_.at(name);
  }

  const Room& GetRoom(int room_id) const { return rooms_.at(room_id); }

  const std::vector<Door>& GetDoors() const { return doors_; }

  const Door& GetDoor(int door_id) const { return doors_.at(door_id); }

  const Panel& GetPanel(int panel_id) const { return panels_.at(panel_id); }

  int GetRoomForPainting(const std::string& painting_id) const {
    return room_by_painting_.at(painting_id);
  }

  const std::vector<int>& GetAchievementPanels() const {
    return achievement_panels_;
  }

 private:
  int AddOrGetRoom(std::string room);
  int AddOrGetDoor(std::string room, std::string door);
  int AddOrGetPanel(std::string room, std::string panel);
  int AddOrGetArea(std::string area);

  std::vector<Room> rooms_;
  std::vector<Door> doors_;
  std::vector<Panel> panels_;
  std::vector<MapArea> map_areas_;

  std::map<std::string, int> room_by_id_;
  std::map<std::string, int> door_by_id_;
  std::map<std::string, int> panel_by_id_;
  std::map<std::string, int> area_by_id_;

  std::map<std::string, int> room_by_painting_;

  std::vector<int> achievement_panels_;
};

const GameData& GetGameData();

#endif /* end of include guard: GAME_DATA_H_9C42AC51 */
