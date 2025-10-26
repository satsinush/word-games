#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "utils/wordUtils.hpp"

namespace Utils {

std::vector<Word> g_words;

std::filesystem::path getExecutableDir() {
  char buffer[1024];
#ifdef _WIN32
  GetModuleFileNameA(NULL, buffer, sizeof(buffer));
#else
  readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif
  return std::filesystem::path(buffer).parent_path();
}

std::string trimToLower(const std::string &str) {
  std::string trimmed = str;
  trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(),
                                              [](unsigned char ch) {
                                                return !std::isspace(ch);
                                              }));
  trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                trimmed.end());
  std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(), ::tolower);
  return trimmed;
}

// Binary I/O functions for efficient binary serialization
void writeBinary(std::ostream &os, const Word &word) {
  // Write string length first, then string data
  size_t len = word.wordString.size();
  os.write(reinterpret_cast<const char *>(&len), sizeof(len));
  os.write(word.wordString.data(), len);

  // Write other members directly
  os.write(reinterpret_cast<const char *>(&word.score), sizeof(word.score));
  os.write(reinterpret_cast<const char *>(&word.is_scrabble),
           sizeof(word.is_scrabble));
  os.write(reinterpret_cast<const char *>(&word.uniqueLetters),
           sizeof(word.uniqueLetters));
  os.write(reinterpret_cast<const char *>(&word.letterCount),
           sizeof(word.letterCount));
}

void readBinary(std::istream &is, Word &word) {
  // Read string length first, then string data
  size_t len;
  is.read(reinterpret_cast<char *>(&len), sizeof(len));
  word.wordString.resize(len);
  is.read(&word.wordString[0], len);

  // Read other members directly
  is.read(reinterpret_cast<char *>(&word.score), sizeof(word.score));
  is.read(reinterpret_cast<char *>(&word.is_scrabble),
          sizeof(word.is_scrabble));
  is.read(reinterpret_cast<char *>(&word.uniqueLetters),
          sizeof(word.uniqueLetters));
  is.read(reinterpret_cast<char *>(&word.letterCount),
          sizeof(word.letterCount));
}

// Loads words from a CSV file. If csvFile is empty, uses word_scores.csv.
// If useBinaryCache is true, tries to load from/save to .bin file.
// If maxWords is 0, loads all words.
std::vector<Word> loadWords(const std::string &csvFile, bool useBinaryCache,
                            size_t maxWords) {
  if (csvFile.empty() && !g_words.empty()) {
    // Return cached words if already loaded from default CSV or binary
    return g_words;
  }

  std::filesystem::path data_dir = getExecutableDir() / "resources";
  std::filesystem::path csv_file =
      csvFile.empty() ? (data_dir / "word_scores.csv") : (data_dir / csvFile);
  std::filesystem::path bin_file = data_dir / "words.bin";
  std::vector<Word> allWordsVec;

  // Try to load from binary file first (only if using cache and default CSV)
  bool loadedFromBin = false;
  if (useBinaryCache && csvFile.empty()) {
    std::ifstream in(bin_file, std::ios::binary);
    if (in) {
      try {
        size_t n;
        in.read(reinterpret_cast<char *>(&n), sizeof(n));
        allWordsVec.resize(n);
        for (size_t i = 0; i < n; ++i) {
          readBinary(in, allWordsVec[i]);
          if (!in)
            throw std::runtime_error("Read error");
        }
        loadedFromBin = true;
        in.close();
      } catch (...) {
        in.close();
        allWordsVec.clear();
      }
    }
  }

  // If binary loading failed, load from CSV
  if (!loadedFromBin) {
    std::ifstream file(csv_file);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open " << csv_file
                << ". Please ensure it exists.\n";
      g_words = allWordsVec;
      return allWordsVec;
    }

    std::string line;
    // Skip header line
    if (!std::getline(file, line)) {
      std::cerr << "Error: Empty CSV file.\n";
      file.close();
      g_words = allWordsVec;
      return allWordsVec;
    }

    size_t wordCount = 0;
    const size_t maxWordsToLoad = (maxWords == 0) ? SIZE_MAX : maxWords;

    while (std::getline(file, line) && wordCount < maxWordsToLoad) {
      if (line.empty())
        continue;

      // Parse CSV line: word,is_scrabble,final_score
      std::istringstream ss(line);
      std::string word, is_scrabble_str, score_str;

      if (std::getline(ss, word, ',') &&
          std::getline(ss, is_scrabble_str, ',') &&
          std::getline(ss, score_str)) {
        // Clean and validate word
        word = trimToLower(word);
        if (word.empty() ||
            std::any_of(word.begin(), word.end(),
                        [](unsigned char c) { return !std::isalpha(c); })) {
          continue;
        }

        // Parse is_scrabble and score
        bool is_scrabble = (is_scrabble_str == "1");
        double score;
        try {
          score = std::stod(score_str);
        } catch (...) {
          continue; // Skip invalid score entries
        }

        // Calculate unique letters count
        int uniqueLetters = std::set<char>(word.begin(), word.end()).size();

        // Calculate letter count array
        std::array<uint8_t, 26> letterCount = {0};
        for (char c : word) {
          letterCount[c - 'a']++;
        }

        allWordsVec.push_back(
            {word, score, is_scrabble, uniqueLetters, letterCount});
        wordCount++;
      }
    }
    file.close();

    // Save to binary for next time (only if using cache and default CSV)
    if (useBinaryCache && csvFile.empty()) {
      if (!std::filesystem::exists(data_dir)) {
        std::filesystem::create_directories(data_dir);
      }

      std::ofstream out(bin_file, std::ios::binary);
      if (out) {
        size_t n = allWordsVec.size();
        out.write(reinterpret_cast<const char *>(&n), sizeof(n));
        for (const auto &w : allWordsVec) {
          writeBinary(out, w);
        }
        out.close();
      }
    }
  }

  g_words = allWordsVec;
  return allWordsVec;
}
} // namespace Utils