#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"

namespace Utils {
namespace Input {

std::string readLine() {
#ifdef _WIN32
  std::string line;
  std::getline(std::cin, line);
  if (std::cin.eof()) {
    std::cin.clear();
    std::cout << "\n";
    throw UserCancelledException();
  }
  return line;
#else
  std::string line;
  struct termios oldt {};
  if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
    // Fallback if stdin is not a TTY
    std::getline(std::cin, line);
    if (std::cin.eof()) {
      std::cin.clear();
      std::cout << "\n";
      throw UserCancelledException();
    }
    return line;
  }

  struct TermiosGuard {
    termios saved {};
    bool active = false;
    explicit TermiosGuard(const termios &oldt) : saved(oldt) {
      termios newt = oldt;
      newt.c_lflag &= ~(ICANON | ECHO);
      if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == 0) {
        active = true;
      }
    }
    ~TermiosGuard() {
      if (active) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved);
      }
    }
  } guard(oldt);

  if (!guard.active) {
    std::getline(std::cin, line);
    if (std::cin.eof()) {
      std::cin.clear();
      std::cout << "\n";
      throw UserCancelledException();
    }
    return line;
  }

  // Session history shared across prompts (newest at back).
  static thread_local std::vector<std::string> history;
  size_t cursor = 0;      // insertion point within line
  size_t histPos = 0;     // 0 = editing draft; 1 = last history entry; ...
  std::string draft;      // line content when browsing history

  auto moveToStart = [&]() {
    while (cursor > 0) {
      std::cout << '\b';
      --cursor;
    }
  };

  auto redrawFromCursor = [&](size_t oldLen) {
    // Print from cursor to end, clear any leftover glyphs, then restore cursor.
    const size_t from = cursor;
    std::cout << line.substr(from);
    if (line.size() < oldLen) {
      std::cout << std::string(oldLen - line.size(), ' ');
      std::cout << std::string(oldLen - line.size(), '\b');
    }
    const size_t trail = line.size() - from;
    std::cout << std::string(trail, '\b') << std::flush;
  };

  auto replaceLine = [&](const std::string &next) {
    const size_t oldLen = line.size();
    moveToStart();
    line = next;
    std::cout << line;
    if (line.size() < oldLen) {
      std::cout << std::string(oldLen - line.size(), ' ');
      std::cout << std::string(oldLen - line.size(), '\b');
    }
    std::cout << std::flush;
    cursor = line.size();
  };

  auto applyHistory = [&](size_t newPos) {
    if (history.empty()) {
      return;
    }
    if (histPos == 0) {
      draft = line;
    }
    histPos = newPos;
    if (histPos == 0) {
      replaceLine(draft);
    } else {
      replaceLine(history[history.size() - histPos]);
    }
  };

  while (true) {
    int ch = getchar();
    if (ch == EOF || ch == 4) { // EOF / Ctrl-D
      std::cout << "\n";
      throw UserCancelledException();
    }
    if (ch == '\n' || ch == '\r') {
      std::cout << "\n";
      break;
    }

    if (ch == 127 || ch == 8) { // Backspace
      if (cursor > 0) {
        const size_t oldLen = line.size();
        --cursor;
        line.erase(cursor, 1);
        std::cout << '\b';
        redrawFromCursor(oldLen);
      }
      continue;
    }

    if (ch == 27) { // Escape / CSI sequence
      int next1 = getchar();
      if (next1 == EOF) {
        continue;
      }
      if (next1 != '[') {
        continue; // Ignore Alt+key / bare Esc
      }

      int next2 = getchar();
      if (next2 == EOF) {
        continue;
      }

      if (next2 == 'A') { // Up — older history
        if (histPos < history.size()) {
          applyHistory(histPos + 1);
        }
      } else if (next2 == 'B') { // Down — newer history / draft
        if (histPos > 0) {
          applyHistory(histPos - 1);
        }
      } else if (next2 == 'C') { // Right
        if (cursor < line.size()) {
          std::cout << line[cursor] << std::flush;
          ++cursor;
        }
      } else if (next2 == 'D') { // Left
        if (cursor > 0) {
          std::cout << '\b' << std::flush;
          --cursor;
        }
      } else if (next2 == 'H') { // Home
        moveToStart();
        std::cout << std::flush;
      } else if (next2 == 'F') { // End
        while (cursor < line.size()) {
          std::cout << line[cursor];
          ++cursor;
        }
        std::cout << std::flush;
      } else if (std::isdigit(next2)) {
        // Extended sequences: ESC [ n ~  (and ESC [ n ; m R etc.)
        std::string params(1, static_cast<char>(next2));
        int c = getchar();
        while (c != EOF && c != '~' && !(c >= 0x40 && c <= 0x7E)) {
          params.push_back(static_cast<char>(c));
          c = getchar();
        }
        if (c == '~') {
          if (params == "3" && cursor < line.size()) { // Delete
            const size_t oldLen = line.size();
            line.erase(cursor, 1);
            redrawFromCursor(oldLen);
          } else if (params == "1" || params == "7") { // Home
            moveToStart();
            std::cout << std::flush;
          } else if (params == "4" || params == "8") { // End
            while (cursor < line.size()) {
              std::cout << line[cursor];
              ++cursor;
            }
            std::cout << std::flush;
          }
        }
      }
      continue;
    }

    if (ch >= 32 && ch <= 126) {
      const size_t oldLen = line.size();
      line.insert(cursor, 1, static_cast<char>(ch));
      redrawFromCursor(oldLen);
      std::cout << line[cursor] << std::flush;
      ++cursor;
    }
  }

  if (!line.empty() && (history.empty() || history.back() != line)) {
    history.push_back(line);
    constexpr size_t kMaxHistory = 100;
    if (history.size() > kMaxHistory) {
      history.erase(history.begin());
    }
  }

  return line;
#endif
}

int promptInt(const std::string &prompt, const int defaultValue, const int min,
              const int max) {
  while (true) {
    std::cout << prompt << " (default: " << defaultValue << "): ";
    std::string input = readLine();

    input = Utils::trimToLower(input);

    if (input.empty())
      return defaultValue;

    try {
      int value = std::stoi(input);
      if (value < min || (max != INT_MAX && value > max)) {
        std::cout << "Value must be between " << min << " and " << max << ".\n";
        continue;
      }
      return value;
    } catch (...) {
      std::cout << "Please enter a valid integer.\n";
    }
  }
}

bool promptBool(const std::string &prompt, const bool defaultValue) {
  while (true) {
    std::cout << prompt << " (default: " << (defaultValue ? "yes" : "no")
              << "): ";
    std::string input = readLine();

    input = Utils::trimToLower(input);

    if (input.empty())
      return defaultValue;

    if (input == "0" || input == "n" || input == "no" || input == "f" ||
        input == "false")
      return false;
    if (input == "1" || input == "y" || input == "yes" || input == "t" ||
        input == "true")
      return true;

    std::cout << "Please enter yes/no, y/n, true/false, or 1/0.\n";
  }
}

std::string promptString(const std::string &prompt,
                         const std::string &defaultValue,
                         std::function<bool(const std::string &)> validator) {
  while (true) {
    if (!defaultValue.empty())
      std::cout << prompt << " (default: " << defaultValue << "): ";
    else
      std::cout << prompt << ": ";

    std::string input = readLine();

    if (input.empty() && !defaultValue.empty())
      return defaultValue;

    if (!validator || validator(input))
      return input;

    std::cout << "Invalid input. Please try again.\n";
  }
}

std::string promptLetters(const std::string &prompt, const size_t expectedCount,
                          const bool allowDuplicates) {
  while (true) {
    std::cout << prompt << std::endl;
    std::string input = readLine();

    // Remove all whitespace
    input.erase(std::remove_if(input.begin(), input.end(), ::isspace),
                input.end());

    if (input.size() != expectedCount) {
      std::cout << "Invalid input. Please enter exactly " << expectedCount
                << " letters." << std::endl;
      continue;
    }

    bool valid = true;
    std::set<char> seen;

    for (size_t i = 0; i < expectedCount; ++i) {
      char c =
          static_cast<char>(std::tolower(static_cast<unsigned char>(input[i])));

      if (!isalpha(static_cast<unsigned char>(input[i]))) {
        std::cout << "All characters must be letters." << std::endl;
        valid = false;
        break;
      }

      if (!allowDuplicates && seen.count(c)) {
        std::cout << "Duplicate letters not allowed." << std::endl;
        valid = false;
        break;
      }

      seen.insert(c);
    }

    if (valid) {
      // Convert to lowercase
      std::transform(input.begin(), input.end(), input.begin(), ::tolower);
      return input;
    }
  }
}

CommandArgs parseCommandArgs(int argc, char *argv[]) {
  CommandArgs result;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    // Handle long options (--option)
    if (arg.substr(0, 2) == "--") {
      std::string key = arg.substr(2);
      std::string value;

      // Check if next argument exists and is not a flag
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        value = argv[i + 1];
        i++; // Skip next argument as it's the value
      } else {
        value = "true"; // Flag without value defaults to true
      }

      result.flags[key] = value;
    }
    // Handle short options (-o)
    else if (arg.size() >= 2 && arg[0] == '-' && arg[1] != '-') {
      // Short option(s)
      for (size_t j = 1; j < arg.size(); ++j) {
        char opt = arg[j];
        std::string key(1, opt);

        // If it's the last character and there's a next arg that's not a flag,
        // take it as value
        if (j == arg.size() - 1 && i + 1 < argc && argv[i + 1][0] != '-') {
          result.flags[key] = argv[i + 1];
          i++;
        } else {
          result.flags[key] = "true";
        }
      }
    }
    // Handle positional arguments
    else {
      result.positional.push_back(arg);
    }
  }

  return result;
}

// Template specializations
template <>
int getArgValue(const std::map<std::string, std::string> &args,
                const std::string &key, const int &defaultValue) {
  auto it = args.find(key);
  if (it == args.end())
    return defaultValue;

  try {
    return std::stoi(it->second);
  } catch (...) {
    return defaultValue;
  }
}

template <>
bool getArgValue(const std::map<std::string, std::string> &args,
                 const std::string &key, const bool &defaultValue) {
  auto it = args.find(key);
  if (it == args.end())
    return defaultValue;

  std::string value = it->second;
  std::string valueLower = Utils::trimToLower(value);

  // Accept various forms of false
  if (valueLower == "0" || valueLower == "false" || valueLower == "no" ||
      valueLower == "f" || valueLower == "n")
    return false;

  // Accept various forms of true (case-insensitive)
  if (valueLower == "1" || valueLower == "true" || valueLower == "yes" ||
      valueLower == "t" || valueLower == "y")
    return true;

  return defaultValue;
}

template <>
std::string getArgValue(const std::map<std::string, std::string> &args,
                        const std::string &key,
                        const std::string &defaultValue) {
  auto it = args.find(key);
  return (it != args.end()) ? it->second : defaultValue;
}

template <>
unsigned int getArgValue(const std::map<std::string, std::string> &args,
                         const std::string &key,
                         const unsigned int &defaultValue) {
  auto it = args.find(key);
  if (it == args.end())
    return defaultValue;

  try {
    return static_cast<unsigned int>(std::stoul(it->second));
  } catch (...) {
    return defaultValue;
  }
}
} // namespace Input
} // namespace Utils