#ifndef TRACKER_STATE_H_8639BC90
#define TRACKER_STATE_H_8639BC90

#include <map>
#include <tuple>

class TrackerState {
 public:
  void CalculateState();

  bool IsLocationReachable(int area_id, int section_id);

 private:
  std::map<std::tuple<int, int>, bool> reachability_;
};

TrackerState& GetTrackerState();

#endif /* end of include guard: TRACKER_STATE_H_8639BC90 */
