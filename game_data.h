#ifndef GAME_DATA_H_9C42AC51
#define GAME_DATA_H_9C42AC51

#include <map>
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
  int room;
  std::string name;
  std::vector<LingoColor> colors;
  std::vector<int> required_rooms;
  std::vector<int> required_doors;
  bool check = false;
  bool exclude_reduce = false;
};

struct Door {
  int room;
  std::string name;
  std::string location_name;
  std::string item_name;
  std::string group_name;
  bool skip_location = false;
  std::vector<int> panels;
  bool exclude_reduce = true;
};

struct Exit {
  int destination_room;
  std::optional<int> door;
};

struct Room {
  std::string name;
  std::vector<Exit> exits;
};

struct Location {
  std::string name;
  int location_id;
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
};

const GameData& GetGameData();

#endif /* end of include guard: GAME_DATA_H_9C42AC51 */
