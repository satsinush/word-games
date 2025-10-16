#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "utils/profilerUtils.hpp"

namespace Utils {

namespace Profiling {

Profiler g_profiler;
Process g_process;

// returns time in nanoseconds
int64_t getTime() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string getDatetime(const int plusSeconds) {
  std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();
  time_t now_c = std::chrono::system_clock::to_time_t(
      now + std::chrono::seconds(plusSeconds));

  time_t tt;
  struct tm *ti;
  time(&tt);
  // ti = localtime(&tt);
  ti = localtime(&now_c);
  std::string date = asctime(ti);
  date = std::regex_replace(date, std::regex("\n"), "");
  return (date);
}

// --- Process ---
Process::Process() {
  lastPrint = getTime();
  this->lastMessageSize = 0;
}

void Process::printUpdate(const std::string &message) {
  int64_t time = getTime();
  lastPrint = time;
  clearLine();
  std::cout << message;
  lastMessageSize = message.size();
}

void Process::clearLine() {
  std::cout << "\r" << std::string(lastMessageSize + 1, ' ') << "\r";
}

void Process::start() {
  this->startTime = getTime();
  lastPrint = this->startTime;
}

void Process::stop() { clearLine(); }

std::string Process::formatSeconds(int64_t totalNano) const {

  int days = totalNano / 86400000000000;
  totalNano = totalNano % 86400000000000;

  int hours = totalNano / 3600000000000;
  totalNano = totalNano % 3600000000000;

  int minutes = totalNano / 60000000000;
  totalNano = totalNano % 60000000000;

  double seconds = totalNano / 1000000000.0;

  std::string s = "";
  if (days > 0) {
    s.append(std::format("{}d ", days));
  }
  if (hours > 0) {
    s.append(std::format("{}h ", hours));
  }
  if (minutes > 0) {
    s.append(std::format("{}m ", minutes));
  }
  s.append(std::format("{:.0f}s ", seconds));

  return (s);
}

int64_t Process::getTimeRemaining(const double progress) const {
  return (((getTime() - startTime) / (progress - 0)) * (1 - progress));
}

void Process::update(double progress, double delay) {
  Utils::Profiling::ProfileScope scope(Utils::Profiling::g_profiler,
                                       "[PROCESS UPDATE]");
  if (progress <= 0) {
    progress = 0;
  }
  int64_t time = getTime();
  if (time - lastPrint > (delay * SEC_TO_NANO)) {
    printUpdate(std::format("Progress: {:.2f}% Time remaining: {}",
                            progress * 100,
                            formatSeconds(getTimeRemaining(progress))));
    lastPrint = time;
  }
}

FunctionProfile::FunctionProfile(const std::string &name,
                                 FunctionProfile *parent) {
  this->functionName = name;
  this->parent = parent;
  this->childProfileMap = std::unordered_map<std::string, FunctionProfile>();
  this->childList = std::vector<FunctionProfile *>();
  this->startTime = 0;
  this->count = 0;
  this->totalTime = 0;
  this->recursionDepth = 0;
  this->maxRecursionDepth = 0;
}

void FunctionProfile::reset() {
  this->childProfileMap.clear();
  this->childList.clear();
  this->startTime = 0;
  this->count = 0;
  this->totalTime = 0;
  this->recursionDepth = 0;
  this->maxRecursionDepth = 0;
}

void FunctionProfile::profileStart(int64_t now) {
  this->startTime = now;
  this->count++;
  this->recursionDepth++;
  if (this->recursionDepth > this->maxRecursionDepth) {
    this->maxRecursionDepth = this->recursionDepth;
  }
}

void FunctionProfile::profileStop(int64_t now) {
  if (this->recursionDepth > 1) {
    // Still in recursion, just decrement depth
    this->recursionDepth--;
  } else {
    // Last recursive call, record timing and reset depth
    this->totalTime += now - this->startTime;
    this->recursionDepth = 0;
  }
}

FunctionProfile *FunctionProfile::getOrCreateChild(const std::string &name) {
  auto it = this->childProfileMap.find(name);
  if (it != this->childProfileMap.end()) {
    return &it->second;
  } else {
    // Create new child profile
    auto insertIt = this->childProfileMap.emplace_hint(
        it, name, FunctionProfile(name, this));
    FunctionProfile *newChild = &insertIt->second;
    this->childList.push_back(newChild);
    return newChild;
  }
}

// --- Profiler ---
Profiler::Profiler() : main("[MAIN]", nullptr) {
  endTime = 0;
  this->start();
}

void Profiler::log(const std::string &message) {
  std::ofstream logFile;
  if (this->logDirectory == "") {
    logFile.open("log.txt", std::ios::app);
  } else {
    logFile.open(this->logDirectory + "\\log.txt", std::ios::app);
  }
  logFile << std::fixed << std::setprecision(9);
  logFile << message << "\n";
  logFile.close();
}

void Profiler::profileStart(const std::string &functionName) {
  int64_t currentTime = getTime();
  // Check if this is a recursive call (same function as current)
  // This optimization prevents creating duplicate child profiles for recursive
  // functions
  if (currentProfile->getName() == functionName) {
    // Recursive call - don't create new profile, just use existing one
    currentProfile->profileStart(currentTime);
  } else {
    // Normal function start - find or create child profile
    FunctionProfile *targetProfile =
        currentProfile->getOrCreateChild(functionName);

    // Start timing for this function
    targetProfile->profileStart(currentTime);

    // Move down the tree to the new current function
    currentProfile = targetProfile;
  }
  this->profilerUpdater->incrementCount();
  this->profilerUpdater->addToTotalTime(getTime() - currentTime);
}

void Profiler::profileStop(const std::string &functionName) {
  int64_t currentTime = getTime();
  // Function end - check if we're ending the current function
  if (currentProfile->getName() == functionName) {
    currentProfile->profileStop(getTime());

    // Move back up the tree to the parent function (only if not recursive)
    if (currentProfile->getRecursionDepth() == 0) {
      currentProfile = currentProfile->getParent();
    }
  }
  this->profilerUpdater->incrementCount();
  this->profilerUpdater->addToTotalTime(getTime() - currentTime);
}

void Profiler::start() {
  this->startTime = getTime();
  this->main.reset();

  this->profilerUpdater = this->main.getOrCreateChild("[PROFILER]");

  this->currentProfile = &this->main; // Start at the main profile
  this->running = true;
}

void Profiler::stop() {
  this->endTime = getTime();

  // Clean up any remaining functions by walking back up the tree
  while (currentProfile != &main && currentProfile != nullptr) {
    currentProfile->profileStop(this->endTime);
    currentProfile = currentProfile->getParent();
  }
  this->running = false;
}

void Profiler::logProfile(const FunctionProfile &profile,
                          const int64_t totalTime, int depth,
                          const bool corner) {
  const FunctionProfile *parent_ptr = profile.getParent();
  if (parent_ptr == nullptr) {
    parent_ptr = &profile; // Avoid null dereference for root
  }
  const FunctionProfile &parent = *parent_ptr;

  uint32_t indentNumChars = depth * 4;
  std::string indent = "";
  if (depth > 0) {
    indent = std::string((depth - 1) * 4, ' ');
    if (corner)
      indent += "  └─";
    else
      indent += "  ├─";
  }

  double average =
      (profile.getCount() == 0)
          ? 0.0
          : (static_cast<double>(profile.getTotalTime()) / profile.getCount());
  double averageMs = average * NANO_TO_SEC * 1000.0;
  double totalSec = profile.getTotalTime() * NANO_TO_SEC;
  double relativePercent =
      (parent.getTotalTime() == 0)
          ? 0.0
          : (100.0 * static_cast<double>(profile.getTotalTime()) /
             parent.getTotalTime());
  double totalPercent =
      (totalTime == 0)
          ? 0.0
          : (100.0 * static_cast<double>(profile.getTotalTime()) / totalTime);

  uint32_t nameFieldWidth = 40;
  uint32_t nameStrWidth = nameFieldWidth - indentNumChars;

  std::string nameStr = profile.getName();
  if (nameStr.size() > nameStrWidth)
    nameStr = nameStr.substr(0, nameStrWidth - 3) + "...";

  std::string nameField =
      indent + nameStr + std::string(nameStrWidth - nameStr.size(), ' ');

  // subsequent columns: each width 10
  log(std::format("{} {:>15.6f} {:>10} {:>10} {:>10.4f} {:>9.2f}% {:>9.2f}%",
                  nameField, averageMs, profile.getCount(),
                  profile.getMaxRecursionDepth() +
                      1, // +1 to count the initial call
                  totalSec, relativePercent, totalPercent));

  const auto &children = profile.getChildren();
  uint32_t numChild = children.size();
  for (uint32_t i = 0; i < numChild; i++) {
    FunctionProfile &child = *children[i];
    bool is_corner = (i == numChild - 1) || !child.getChildren().empty();
    logProfile(child, totalTime, depth + 1, is_corner);
  }
}

void Profiler::logProfilerData() {
  int64_t totalRunTime = (this->endTime) - (this->startTime);
  this->main.setTotalTime(totalRunTime);
  this->main.incrementCount();

  log(std::format("{:<40} {:>15} {:>10} {:>10} {:>10} {:>10} {:>10}",
                  "FUNCTION", "AVG (ms)", "COUNT", "DEPTH", "TOTAL (s)",
                  "% PARENT", "% TOTAL"));
  logProfile(main, totalRunTime);
  log("");
}

int64_t Profiler::getTotalTime() const {
  return this->endTime - this->startTime;
}

ProfileScope::ProfileScope(Profiler &profiler, const std::string &name)
    : profiler(profiler), functionName(name) {
  if (!profiler.isRunning())
    return;
  profiler.profileStart(functionName);
}

ProfileScope::~ProfileScope() {
  if (!profiler.isRunning())
    return;
  profiler.profileStop(functionName);
}
} // namespace Profiling
} // namespace Utils