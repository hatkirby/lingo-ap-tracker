#ifndef AP_STATE_H_664A4180
#define AP_STATE_H_664A4180

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>

#include "game_data.h"
#include "tracker_frame.h"

enum DoorShuffleMode { kNO_DOORS = 0, kSIMPLE_DOORS = 1, kCOMPLEX_DOORS = 2 };

class APState {
 public:
  APState();

  void SetTrackerFrame(TrackerFrame* tracker_frame) {
    tracker_frame_ = tracker_frame;
  }

  void Connect(std::string server, std::string player, std::string password);

  bool HasCheckedGameLocation(int area_id, int section_id) const;

  bool HasColorItem(LingoColor color) const;

  bool HasItem(const std::string& item, int quantity = 1) const;

  DoorShuffleMode GetDoorShuffleMode() const { return door_shuffle_mode_; }

  bool IsColorShuffle() const { return color_shuffle_; }

  bool IsPaintingShuffle() const { return painting_shuffle_; }

  const std::map<std::string, std::string> GetPaintingMapping() const {
    return painting_mapping_;
  }

 private:
  void RefreshTracker();

  int64_t GetItemId(const std::string& item_name);

  void DestroyClient();

  TrackerFrame* tracker_frame_;

  bool client_active_ = false;
  std::mutex client_mutex_;

  bool connected_ = false;
  bool has_connection_result_ = false;

  std::map<int64_t, int> inventory_;
  std::set<int64_t> checked_locations_;

  std::map<std::tuple<int, int>, int64_t> ap_id_by_location_id_;
  std::map<std::string, int64_t> ap_id_by_item_name_;
  std::map<LingoColor, int64_t> ap_id_by_color_;
  std::map<int64_t, std::string> progressive_item_by_ap_id_;

  DoorShuffleMode door_shuffle_mode_ = kNO_DOORS;
  bool color_shuffle_ = false;
  bool painting_shuffle_ = false;

  std::map<std::string, std::string> painting_mapping_;
};

APState& GetAPState();

#endif /* end of include guard: AP_STATE_H_664A4180 */
