#ifndef TRACKER_CONFIG_H_36CDD648
#define TRACKER_CONFIG_H_36CDD648

#include <string>

class TrackerConfig {
 public:
  void Load();

  void Save();

  std::string ap_server;
  std::string ap_player;
  std::string ap_password;
  bool asked_to_check_for_updates = false;
  bool should_check_for_updates = false;
};

TrackerConfig& GetTrackerConfig();

#endif /* end of include guard: TRACKER_CONFIG_H_36CDD648 */
