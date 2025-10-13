#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Utils {
std::filesystem::path getExecutableDir();

// Represents a word with its properties
struct Word {
  // The word string in lowercase
  std::string wordString;
  // The score associated with the word
  double score;
  // Whether the word is valid in Scrabble
  bool is_scrabble;
  // Number of unique letters in the word
  int uniqueLetters;
  // Count of each letter a-z
  std::array<uint8_t, 26> letterCount;
  // Comparison operator for sorting words alphabetically
  bool operator<(const Word &other) const {
    if (score != other.score)
      return score > other.score; // Higher score first
    return wordString < other.wordString;
  }
  // Equality operator for comparing words
  bool operator==(const Word &other) const {
    return wordString == other.wordString;
  }
};

// Binary stream operators for efficient binary I/O
void writeBinary(std::ostream &os, const Word &word);
void readBinary(std::istream &is, Word &word);

// Trims whitespace and converts a string to lowercase
std::string trimToLower(const std::string &str);

// Loads words from words.bin if available, otherwise from word_scores.csv and
// saves to words.bin.
std::vector<Word> loadWords();

} // namespace Utils

// Provide std::hash specialization for Utils::Word so it can be used as an
// unordered_map/unordered_set key in generic code (like the EntropySolver).
namespace std {
template <> struct hash<Utils::Word> {
  size_t operator()(const Utils::Word &word) const noexcept {
    // Use the standard string hash for the wordString
    return std::hash<std::string>{}(word.wordString);
  }
};
} // namespace std