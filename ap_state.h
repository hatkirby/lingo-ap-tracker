#ifndef AP_STATE_H_664A4180
#define AP_STATE_H_664A4180

#include <apclient.hpp>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "game_data.h"
#include "tracker_frame.h"

class APState {
 public:
  APState();

  void SetTrackerFrame(TrackerFrame* tracker_frame) {
    tracker_frame_ = tracker_frame;
  }

  void Connect(std::string server, std::string player, std::string password);

 private:
  TrackerFrame* tracker_frame_;

  std::unique_ptr<APClient> apclient_;
  bool client_active_ = false;
  std::mutex client_mutex_;

  std::set<int> inventory_;
  std::set<int> checked_locations_;

  std::map<int, int> ap_id_by_location_id_;
  std::map<int, int> ap_id_by_door_id_;
  std::map<int, int> ap_id_by_door_group_id_;
  std::map<LingoColor, int> ap_id_by_color_;
};

APState& GetAPState();

#endif /* end of include guard: AP_STATE_H_664A4180 */
