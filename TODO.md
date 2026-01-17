## Tasks to do and future improvements
- Fix debug heap corruption error and segmentation faults when using Tracy
  - Update Tracy to use proper thread handling
- Add support for more games
  - Waffle
  - Strands
- Hangman number of words output doesn't match actual number of words
- If a letter was already guessed and isn't in a word, it should be used to filter out words that contain that letter (if I was guessed and is in another word or not in any and the pattern is T?E ???? TIE should not be in the list of possible words)