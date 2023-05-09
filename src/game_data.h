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
  bool exclude_reduce = false;
};

struct MapArea {
  int id;
  std::string name;
  std::vector<Location> locations;
  int map_x;
  int map_y;
  bool exclude_reduce = false;
};

const std::vector<MapArea>& GD_GetMapAreas();
const MapArea& GD_GetMapArea(int id);
int GD_GetRoomByName(const std::string& name);
const Room& GD_GetRoom(int room_id);
const std::vector<Door>& GD_GetDoors();
const Door& GD_GetDoor(int door_id);
const Panel& GD_GetPanel(int panel_id);
int GD_GetRoomForPainting(const std::string& painting_id);
const std::vector<int>& GD_GetAchievementPanels();

#endif /* end of include guard: GAME_DATA_H_9C42AC51 */
