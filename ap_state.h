#ifndef AP_STATE_H_664A4180
#define AP_STATE_H_664A4180

#include <apclient.hpp>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>

#include "game_data.h"
#include "tracker_frame.h"

enum class DoorShuffleMode { kNone, kSimple, kComplex };

class APState {
 public:
  APState();

  void SetTrackerFrame(TrackerFrame* tracker_frame) {
    tracker_frame_ = tracker_frame;
  }

  void Connect(std::string server, std::string player, std::string password);

  bool HasCheckedGameLocation(int area_id, int section_id) const;

  bool HasColorItem(LingoColor color) const;

  DoorShuffleMode GetDoorShuffleMode() const { return door_shuffle_mode_; }

  bool IsColorShuffle() const { return color_shuffle_; }

 private:
  void RefreshTracker();

  int64_t GetItemId(const std::string& item_name);

  TrackerFrame* tracker_frame_;

  std::unique_ptr<APClient> apclient_;
  bool client_active_ = false;
  std::mutex client_mutex_;

  std::set<int64_t> inventory_;
  std::set<int64_t> checked_locations_;

  std::map<std::tuple<int, int>, int64_t> ap_id_by_location_id_;
  std::map<int, int64_t> ap_id_by_door_id_;
  std::map<int, int64_t> ap_id_by_door_group_id_;
  std::map<LingoColor, int64_t> ap_id_by_color_;

  DoorShuffleMode door_shuffle_mode_ = DoorShuffleMode::kNone;
  bool color_shuffle_ = false;
};

APState& GetAPState();

#endif /* end of include guard: AP_STATE_H_664A4180 */
