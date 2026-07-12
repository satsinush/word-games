# Puzzle++ Usage Guide

This document outlines the complete usage instructions for the Puzzle++ application, covering both the Command Line Interface (CLI) and the Graphical User Interface (GUI).

## Launching the Application

The program can be launched in three ways:

1.  **GUI Mode:** Run the executable with no arguments (requires GUI compilation support).
2.  **Interactive CLI:** Use the `-i` flag to enter a session-based command line.
3.  **Direct Command:** Pass specific game modes and options as arguments.

---

# Part 1: GUI Usage

## Wordle

![Wordle screenshot](./screenshots/wordle.png)

1.  **Configuration (⚙️):**
      * **Word Length:** Default is 5.
      * **Search Depth:** `0` (Fast, Probability only), `1` (Balanced), `2+` (Slow, High Accuracy).
      * **Exclude Uncommon:** Recommended for standard Wordle to filter non-Scrabble words.
2.  **Playing:**
      * Enter a word and press **Enter** or **Add Word**.
      * Click the letters in the history to cycle feedback colors:
          * ⬛ **Gray:** Incorrect.
          * 🟨 **Yellow:** Wrong position.
          * 🟩 **Green:** Correct position.
      * Click **Solve** to generate suggestions.
3.  **Results Table:**
      * **ENT (Entropy):** Estimated turns to solve. `1.0` means the next guess guarantees a solution. Lower is better (0.0 is the solution).
      * **Word Score:** Heuristic tie-breaker based on frequency/length.
      * **Probability:** Likelihood of the word being the specific answer.

## Spelling Bee

![Spelling Bee screenshot](./screenshots/spellingbee.png)

1.  **Configuration (⚙️):**
      * **Exclude Uncommon:** Filter out obscure words.
      * **Must Include First:** Required for standard Spelling Bee.
      * **Reuse Letters:** Toggle `false` to play "Anagram" mode (use letters exactly once).
2.  **Playing:**
      * Enter letters. **The first letter entered is the special center letter.**
      * The table updates automatically with all valid words and their unique letter counts.

## Letter Boxed

![Letter Boxed screenshot](./screenshots/letterboxed.png)

1.  **Configuration (⚙️):**
      * **Preset:** Choose `Default`, `Fast`, or `Thorough` based on your time constraints.
      * **Max Words:** Limit the solution length (Standard is 2).
      * **Pruning:** Keep "Prune Redundant Paths" enabled for speed.
2.  **Playing:**
      * Enter the 12 letters as a continuous string.
      * Press **Enter** to solve.
      * The table displays solutions and the number of words used.

## Mastermind

![Mastermind screenshot](./screenshots/mastermind.png)

1.  **Configuration (⚙️):**
      * **Pegs:** Number of slots in the board.
      * **Colors:** The characters available for guessing (Default: `RGBCMY`).
2.  **Playing:**
      * Type a guess using your color characters (e.g., `RGBC`).
      * **Feedback Input:**
          * **Positions (Black Pegs):** Correct color, correct place.
          * **Colors (White Pegs):** Correct color, wrong place.
      * Click **Solve** to calculate the best next move based on entropy.

## Hangman

1.  **Configuration (⚙️):**
      * **Search Depth:** `0` (Fast), `1` (Balanced), `2+` (High Accuracy).
      * **Exclude Uncommon:** Filter non-Scrabble words from suggestions.
2.  **Playing:**
      * Enter the word pattern using `_` for unknown letters and actual letters for revealed positions.
      * For multi-word phrases, separate patterns with spaces (e.g., `_A__ ___ _____`).
      * Add guessed letters and mark whether they appear in the word (✓) or not (✗).
      * Click **Solve** to get the best letter suggestions.
3.  **Results Table:**
      * **Letter:** The suggested letter to guess next.
      * **ENT (Entropy):** Expected turns remaining. Lower is better.
      * **In Word %:** Probability that the letter appears in the word.
4.  **Possible Words:**
      * Shows sample words that match the current pattern and feedback constraints.

## Dungleon

![Dungleon screenshot](./screenshots/dungleon.png)

1.  **Input:**
      * Click the character icons to build your guess.
      * Click the characters *in the guess list* to cycle their state:
          * 🟥 **Red:** Not in solution.
          * 🟨 **Yellow:** In solution, wrong pos (no more instances).
          * 🟩 **Green:** Correct pos (no more instances).
          * 🟨➕ **Yellow (+):** In solution, wrong pos (more instances exist).
          * 🟩➕ **Green (+):** Correct pos (more instances exist).
2.  **Gauntlet Mode:**
      * Use **Submit Solution** to add a row as a "Past Solution" (used for Gauntlet constraints).
      * Use **Submit Guess** for standard feedback rows.
3.  **Solving:**
      * Click **Solve** to view the Top Suggestions sorted by Entropy (ENT).


-----

# Part 2: Command Line Interface (CLI)

**General Syntax:**
```bash
./p++ [OPTIONS] [MODE]
```

### Global Options

  * `-h`, `--help`: Displays the help message.
  * `-v`, `--verbose`: Enables verbose output for detailed logging (reserved for future use).
  * `-i`: Enters Interactive CLI mode.

### Boolean Values

For any CLI option that accepts a `<bool>`, the following inputs are valid:

  * **True:** `1`, `y`, `Y`, `yes`, `YES`, `Yes`, `t`, `T`, `true`, `True`, `TRUE`
  * **False:** `0`, `n`, `N`, `no`, `NO`, `No`, `f`, `F`, `false`, `False`, `FALSE`

-----

## Game Modes

### 1\. Wordle

Generates entropy-based suggestions for Wordle.

**Syntax:**

```bash
./p++ wordle [OPTIONS]
```

**Options:**

  * `--guesses <string>`: A semicolon-separated string of `WORD FEEDBACK` pairs.
      * **Feedback Codes:** `0` (Grey), `1` (Yellow), `2` (Green).
      * **Example:** `"AUDIO 00100;SOARE 10201"`
  * `--word-length <n>`: Length of the puzzle word. (Default: `5`).
  * `--max-depth <0-2>`: Search depth for entropy. Higher is more accurate but slower. (Default: `0`).
  * `--auto-depth`: Dynamically calculate optimal depth based on candidate complexity. (Overrides `--max-depth`).
  * `--exclude-uncommon-words <bool>`: Limit search to common words. (Default: `false`).
  * `-o`, `--output <file>`: Output file path. (Default: `results/guesses.txt`).

### 2\. Spelling Bee

Solves the Spelling Bee puzzle.

**Syntax:**

```bash
./p++ spellingbee --letters <letters> [OPTIONS]
```

**Options:**

  * `--letters <string>`: **(Required)** Minimum 3 letters. **The first letter is the "Center" (special) letter.**
  * `--exclude-uncommon-words <bool>`: Exclude obscure words. (Default: `false`).
  * `--must-include-first-letter <bool>`: Require words to use the center letter. (Default: `true`).
  * `--reuse-letters <bool>`: Allow infinite reuse of letters. (Default: `true`).
  * `-o`, `--output <file>`: Output file path. (Default: `results/temp.txt`).

### 3\. Letter Boxed

Solves the Letter Boxed puzzle by finding paths connecting letters on a square.

**Syntax:**

```bash
./p++ letterboxed --letters <12letters> [OPTIONS]
```

**Options:**

  * `--letters <string>`: **(Required)** The 12 letters of the box provided as a single string (e.g., `abcdefghijkl`).
  * `--preset <0-3>`: Applies a configuration preset. (Arguments override preset values).
      * `1`: **Default** (Balanced).
      * `2`: **Fast** (Speed over completeness).
      * `3`: **Thorough** (Comprehensive search, slower).
      * `0`: **Custom** (Uses defaults, fully overridable).
  * `--max-depth <n>`: Max number of words allowed in a solution.
  * `--min-word-length <n>`: Minimum length for a valid word.
  * `--min-unique-letters <n>`: Minimum unique letters a word must contain.
  * `--prune-paths <bool>`: **(Recommended)** Stop processing if a word adds no new letters.
  * `--prune-classes <bool>`: **(Recommended)** Eliminate word classes with fewer unique letters.
  * `-o`, `--output <file>`: Output file path. (Default: `results/temp.txt`).

### 4\. Mastermind

Generates entropy-based suggestions for Mastermind.

**Syntax:**

```bash
./p++ mastermind [OPTIONS]
```

**Options:**

  * `--guesses <string>`: A semicolon-separated string of `CODE BLACK WHITE` pairs.
      * **Format:** `"RGBC 1 2"` (1 Black peg, 2 White pegs).
  * `--pegs <n>`: Number of pegs per code. (Default: `4`).
  * `--colors <string>`: Available color characters. (Default: `"RGBCMY"`).
  * `--allow-duplicates <bool>`: Allow duplicate colors in code. (Default: `true`).
  * `--max-depth <0-2>`: Search depth for entropy. (Default: `1`).
  * `--auto-depth`: Dynamically calculate optimal depth based on candidate complexity. (Overrides `--max-depth`).
  * `-o`, `--output <file>`: Output file path. (Default: `results/guesses.txt`).

### 5\. Hangman

Generates entropy-based letter suggestions for Hangman puzzles.

**Syntax:**

```bash
./p++ hangman [OPTIONS]
```

**Options:**

  * `--input <string>`: Combined pattern and strikes in format `"PATTERN;STRIKES"`.
      * **Format:** `"_A__ ___;xyz"` where pattern is before `;` and strikes (letters NOT in phrase) are after.
      * **Example:** `"_A__ ___;etxzq"` (two words with 'A' revealed, and letters e, t, x, z, q are not in the phrase).
  * `--pattern <string>`: Word pattern(s) using `_` for unknown letters and actual letters for revealed positions (alternative to `--input`).
      * **Single word:** `"_A___"` (5-letter word with 'A' in position 2).
      * **Multi-word phrase:** `"____ ___ _____"` (three words of lengths 4, 3, and 5).
  * `--strikes <string>`: Letters that are NOT in the word/phrase (alternative to `--input`).
      * **Example:** `"etxzq"` (letters e, t, x, z, q have been guessed and are not in the word).
  * `--max-depth <0-2>`: Search depth for entropy calculation. (Default: `0`).
  * `--auto-depth`: Dynamically calculate optimal depth based on candidate complexity. (Overrides `--max-depth`).
  * `--exclude-uncommon-words <bool>`: Limit search to common words. (Default: `false`).
  * `-o`, `--output <file>`: Output file path. (Default: `results/hangman.txt`).

### 6\. Dungleon

Generates entropy-based suggestions for Dungleon.

**Syntax:**

```bash
./p++ dungleon [OPTIONS]
```

**Options:**

  * `--guesses <string>`: Semicolon-separated string of character pairs followed by feedback string.
      * **Format:** `"ar kn ma bt dr 01234"`
      * **Codes:** `0` (Not present), `1` (Wrong pos, no more), `2` (Correct pos, no more), `3` (Wrong pos, one more), `4` (Correct pos, one more).
  * `--solutions <string>`: Past solutions (for Gauntlet mode), separated by semicolons.
      * **Format:** `"ar kn ma bt dr;cl wa ro th pr"`
  * `--max-depth <0-2>`: Search depth for entropy. (Default: `0`).
  * `--auto-depth`: Dynamically calculate optimal depth based on candidate complexity. (Overrides `--max-depth`).
  * `--exclude-impossible <bool>`: Exclude impossible patterns. (Default: `false`).
  * `-o`, `--output <file>`: Output file path. (Default: `results/dungleon.txt`).

### 7\. Read Mode

Reads and displays results from a generated file.

**Syntax:**

```bash
./p++ read [FILE] [OPTIONS]
```

**Options:**

  * `--start <n>`: Starting line index. (Default: `0`).
  * `--end <n>`: Ending line index. (Default: `all`).

-----

## Benchmarking

Benchmarks are a separate `p++-benchmarks` binary (Google Benchmark), built when `BUILD_BENCHMARKS` is enabled. Debug presets such as `linux-debug`, `mingw-debug`, and `msvc-debug` turn this on by default.

```bash
# Configure and build (example: linux-debug)
cmake --preset linux-debug
cmake --build --preset linux-debug

# Run all solver benchmarks
./build/linux-debug/p++-benchmarks

# Report times in milliseconds
./build/linux-debug/p++-benchmarks --benchmark_time_unit=ms

# Run a specific benchmark by name filter
./build/linux-debug/p++-benchmarks --benchmark_filter=BM_Wordle
```

On Windows MinGW builds, use `./build/mingw-debug/p++-benchmarks.exe` instead.

Covered solvers: Wordle, Spelling Bee, Letter Boxed, Mastermind, Dungleon, and Hangman.

-----

## CLI Examples

**Wordle:**

```bash
wordle --word-length 5 --max-depth 1 --exclude-uncommon-words true --guesses "STEAL 20100;CRANE 01002" -o results/wordle.txt
```

**Spelling Bee:**

```bash
spellingbee --letters nhmkace --exclude-uncommon-words false --must-include-first-letter true --reuse-letters true -o results/spellingbee.txt
```

**Letter Boxed:**

```bash
letterboxed --letters uvjswitgebac --preset 0 --max-depth 3 --min-word-length 4 --min-unique-letters 3 --prune-paths true --prune-classes false -o results/letterboxed.txt
```

**Mastermind:**

```bash
mastermind --guesses "RGBC 1 2;MYRC 1 2" --pegs 4 --colors "RGBCMY" --allow-duplicates true --max-depth 1 -o results/mastermind.txt
```

**Hangman:**

```bash
hangman --input "_A___ ___;etz" --max-depth 1 --exclude-uncommon-words true -o results/hangman.txt
```

**Dungleon:**

```bash
dungleon --guesses "ar kn bo ne fr 00010" --solutions "vi zo ne sk bt" --max-depth 0 --exclude-impossible false -o results/dungleon.txt
```