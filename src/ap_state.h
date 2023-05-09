#ifndef AP_STATE_H_664A4180
#define AP_STATE_H_664A4180

#include <map>
#include <string>

#include "game_data.h"

class TrackerFrame;

enum DoorShuffleMode { kNO_DOORS = 0, kSIMPLE_DOORS = 1, kCOMPLEX_DOORS = 2 };

void AP_SetTrackerFrame(TrackerFrame* tracker_frame);

void AP_Connect(std::string server, std::string player, std::string password);

bool AP_HasCheckedGameLocation(int area_id, int section_id);

bool AP_HasColorItem(LingoColor color);

bool AP_HasItem(const std::string& item, int quantity = 1);

DoorShuffleMode AP_GetDoorShuffleMode();

bool AP_IsColorShuffle();

bool AP_IsPaintingShuffle();

const std::map<std::string, std::string> AP_GetPaintingMapping();

int AP_GetMasteryRequirement();

bool AP_IsReduceChecks();

#endif /* end of include guard: AP_STATE_H_664A4180 */
