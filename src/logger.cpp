#include "logger.h"

#include <chrono>
#include <fstream>
#include <mutex>

namespace {

class Logger {
 public:
  Logger() : logfile_("debug.log") {}

  void LogLine(const std::string& text) {
    std::lock_guard guard(file_mutex_);
    logfile_ << "[" << std::chrono::system_clock::now() << "] " << text
             << std::endl;
    logfile_.flush();
  }

 private:
  std::ofstream logfile_;
  std::mutex file_mutex_;
};

}  // namespace

void TrackerLog(const std::string& text) {
  static Logger* instance = new Logger();
  instance->LogLine(text);
}
