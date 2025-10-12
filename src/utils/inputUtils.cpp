#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <set>
#include <climits>

#include "inputUtils.hpp"
#include "wordUtils.hpp"

namespace Utils
{
    namespace Input
    {
        int promptInt(const std::string &prompt, const int defaultValue, const int min, const int max)
        {
            while (true)
            {
                std::cout << prompt << " (default: " << defaultValue << "): ";
                std::string input;
                std::getline(std::cin, input);
                input = Utils::trimToLower(input);

                if (input.empty())
                    return defaultValue;

                try
                {
                    int value = std::stoi(input);
                    if (value < min || (max != INT_MAX && value > max))
                    {
                        std::cout << "Value must be between " << min << " and " << max << ".\n";
                        continue;
                    }
                    return value;
                }
                catch (...)
                {
                    std::cout << "Please enter a valid integer.\n";
                }
            }
        }

        bool promptBool(const std::string &prompt, const bool defaultValue)
        {
            while (true)
            {
                std::cout << prompt << " (default: " << (defaultValue ? "yes" : "no") << "): ";
                std::string input;
                std::getline(std::cin, input);
                input = Utils::trimToLower(input);

                if (input.empty())
                    return defaultValue;

                if (input == "0" || input == "n" || input == "no" || input == "f" || input == "false")
                    return false;
                if (input == "1" || input == "y" || input == "yes" || input == "t" || input == "true")
                    return true;

                std::cout << "Please enter yes/no, y/n, true/false, or 1/0.\n";
            }
        }

        std::string promptString(const std::string &prompt, const std::string &defaultValue,
                                 std::function<bool(const std::string &)> validator)
        {
            while (true)
            {
                if (!defaultValue.empty())
                    std::cout << prompt << " (default: " << defaultValue << "): ";
                else
                    std::cout << prompt << ": ";

                std::string input;
                std::getline(std::cin, input);

                if (input.empty() && !defaultValue.empty())
                    return defaultValue;

                if (!validator || validator(input))
                    return input;

                std::cout << "Invalid input. Please try again.\n";
            }
        }

        std::string promptLetters(const std::string &prompt, const size_t expectedCount, const bool allowDuplicates)
        {
            while (true)
            {
                std::cout << prompt << std::endl;
                std::string input;
                std::getline(std::cin, input);

                // Remove all whitespace
                input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());

                if (input.size() != expectedCount)
                {
                    std::cout << "Invalid input. Please enter exactly " << expectedCount << " letters." << std::endl;
                    continue;
                }

                bool valid = true;
                std::set<char> seen;

                for (size_t i = 0; i < expectedCount; ++i)
                {
                    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(input[i])));

                    if (!isalpha(static_cast<unsigned char>(input[i])))
                    {
                        std::cout << "All characters must be letters." << std::endl;
                        valid = false;
                        break;
                    }

                    if (!allowDuplicates && seen.count(c))
                    {
                        std::cout << "Duplicate letters not allowed." << std::endl;
                        valid = false;
                        break;
                    }

                    seen.insert(c);
                }

                if (valid)
                {
                    // Convert to lowercase
                    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
                    return input;
                }
            }
        }

        std::map<std::string, std::string> parseCommandArgs(int argc, char *argv[])
        {
            std::map<std::string, std::string> args;

            for (int i = 1; i < argc; ++i)
            {
                std::string arg = argv[i];

                if (arg.substr(0, 2) == "--")
                {
                    std::string key = arg.substr(2);
                    std::string value;

                    // Check if next argument exists and is not a flag
                    if (i + 1 < argc && std::string(argv[i + 1]).substr(0, 2) != "--")
                    {
                        value = argv[i + 1];
                        i++; // Skip next argument as it's the value
                    }
                    else
                    {
                        value = "true"; // Flag without value defaults to true
                    }

                    args[key] = value;
                }
            }

            return args;
        }

        // Template specializations
        template <>
        int getArgValue(const std::map<std::string, std::string> &args, const std::string &key, const int &defaultValue)
        {
            auto it = args.find(key);
            if (it == args.end())
                return defaultValue;

            try
            {
                return std::stoi(it->second);
            }
            catch (...)
            {
                return defaultValue;
            }
        }

        template <>
        bool getArgValue(const std::map<std::string, std::string> &args, const std::string &key, const bool &defaultValue)
        {
            auto it = args.find(key);
            if (it == args.end())
                return defaultValue;

            std::string value = Utils::trimToLower(it->second);
            if (value == "0" || value == "false" || value == "no" || value == "f" || value == "n")
                return false;
            if (value == "1" || value == "true" || value == "yes" || value == "t" || value == "y")
                return true;

            return defaultValue;
        }

        template <>
        std::string getArgValue(const std::map<std::string, std::string> &args, const std::string &key, const std::string &defaultValue)
        {
            auto it = args.find(key);
            return (it != args.end()) ? it->second : defaultValue;
        }

        template <>
        unsigned int getArgValue(const std::map<std::string, std::string> &args, const std::string &key, const unsigned int &defaultValue)
        {
            auto it = args.find(key);
            if (it == args.end())
                return defaultValue;

            try
            {
                return static_cast<unsigned int>(std::stoul(it->second));
            }
            catch (...)
            {
                return defaultValue;
            }
        }
    }
}