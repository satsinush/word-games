#pragma once
#include <climits>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Utils {
namespace Input {
// Exception thrown when user cancels input EOF
class UserCancelledException : public std::runtime_error {
public:
  UserCancelledException() : std::runtime_error("User cancelled input") {}
};

// Structure to hold parsed command line arguments
struct CommandArgs {
  std::map<std::string, std::string> flags;
  std::vector<std::string> positional;
};
// Get validated integer input with default value and optional range
int promptInt(const std::string &prompt, const int defaultValue,
              const int min = INT_MIN, const int max = INT_MAX);

// Get validated boolean input (accepts 0/1, y/n, yes/no, true/false)
bool promptBool(const std::string &prompt, const bool defaultValue);

// Get string input with optional validation function
std::string
promptString(const std::string &prompt, const std::string &defaultValue = "",
             std::function<bool(const std::string &)> validator = nullptr);

// Get letters input with specified count and validation
std::string promptLetters(const std::string &prompt, const size_t expectedCount,
                          const bool allowDuplicates = true);

// Read a line of input with arrow keys and backspace editing handled gracefully
std::string readLine();

// Parse command line arguments into flags and positional arguments
CommandArgs parseCommandArgs(int argc, char *argv[]);

// Helper to get value from args map with default
template <typename T>
T getArgValue(const std::map<std::string, std::string> &args,
              const std::string &key, const T &defaultValue);

// Specialized template declarations
template <>
int getArgValue(const std::map<std::string, std::string> &args,
                const std::string &key, const int &defaultValue);
template <>
bool getArgValue(const std::map<std::string, std::string> &args,
                 const std::string &key, const bool &defaultValue);
template <>
std::string getArgValue(const std::map<std::string, std::string> &args,
                        const std::string &key,
                        const std::string &defaultValue);
template <>
unsigned int getArgValue(const std::map<std::string, std::string> &args,
                         const std::string &key,
                         const unsigned int &defaultValue);
} // namespace Input
} // namespace Utils