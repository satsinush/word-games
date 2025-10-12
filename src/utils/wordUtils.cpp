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

#include "wordUtils.hpp"

namespace Utils {
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

// Loads words from words.bin if available, otherwise from word_scores.csv
// (first 300,000 words) and saves to words.bin.
std::vector<Word> loadWords() {
  std::filesystem::path data_dir = getExecutableDir() / "resources";
  std::filesystem::path csv_file = data_dir / "word_scores.csv";
  std::filesystem::path bin_file = data_dir / "words.bin";
  std::vector<Word> allWordsVec;

  // Try to load from binary file first
  std::ifstream in(bin_file, std::ios::binary);
  bool loadedFromBin = false;
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

  // If binary loading failed, load from CSV
  if (!loadedFromBin) {
    std::ifstream file(csv_file);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open word_scores.csv. Please ensure it's "
                   "in the 'data' directory.\n";
      return allWordsVec;
    }

    std::string line;
    // Skip header line
    if (!std::getline(file, line)) {
      std::cerr << "Error: Empty CSV file.\n";
      file.close();
      return allWordsVec;
    }

    constexpr size_t MAX_WORDS = 500002;
    size_t wordCount = 0;

    while (std::getline(file, line) && wordCount < MAX_WORDS) {
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

    // Save to binary for next time
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

  return allWordsVec;
}
} // namespace Utils