# Puzzle++ CLI Usage

This document outlines the usage, options, and modes for the Puzzle++ command-line interface.

---

## General Usage

The program can be launched in several ways:
- **GUI Mode:** Running the executable with no arguments will launch the GUI, provided it was compiled with GUI support.
- **Interactive CLI Mode:** Use the `-i` flag to enter an interactive command-line session.
- **Direct Command Mode:** Specify a game mode and its options directly as arguments.

**Syntax:**
```bash
./solver-cli [OPTIONS] [MODE]
````

### Global Options

  * `-h`, `--help`: Displays the help message.
  * `-v`, `--verbose`: Enables verbose output for detailed logging (currently for future use).
  * `-i`: Runs the program in interactive CLI mode.

-----

## Game Modes

The solver supports several game modes, each with its own set of specific options:

- **Letter Boxed**: Solve Letter Boxed puzzles
- **Spelling Bee**: Solve Spelling Bee puzzles  
- **Wordle**: Get entropy-based suggestions for Wordle puzzles
- **Mastermind**: Get entropy-based suggestions for Mastermind puzzles
- **Dungleon**: Get entropy-based suggestions for Dungleon puzzles
- **Read Mode**: Read and display results from files

### Letter Boxed

Solves the Letter Boxed puzzle.

**Syntax:**

```bash
./solver-cli letterboxed --letters <12letters> [OPTIONS]
```

**Options:**

  * `--letters <letters>`: **(Required)** The 12 letters for the puzzle, provided as a single string (e.g., `abcdefghijkl`).
  * `--preset <1|2|3|0>`: Specifies a preset configuration for the solver. Preset settings are applied first, then any additional arguments override the preset values.
      * `1`: Default - A balanced approach.
      * `2`: Fast - Prioritizes speed over finding every possible solution.
      * `3`: Thorough - A comprehensive search that may take longer.
      * `0`: Custom - Uses default values that can be overridden with the options below.
  * `--max-depth <n>`: The maximum number of words allowed in a single solution. Can override preset value.
  * `--min-word-length <n>`: The minimum length for a valid word. Can override preset value.
  * `--min-unique-letters <n>`: The minimum number of unique letters a word must contain. Can override preset value.
  * `--prune-paths <bool>`: **(Recommended)** Stops processing paths if a word is added that doesn't cover any new letters. Can override preset value. This is always done on words that start and end on the same letter but may exclude very specific situations where a short word is needed to switch starting letters.
  * `--prune-classes <bool>`: **(Recommended)** Eliminates classes of words that have fewer unique letters. Can override preset value. This may exclude solutions that use more common words in favor of using word classes that cover more letters, even if those words are less common.
  * `-o`, `--output <file>`: Specifies the output file path. (Default: `results/temp.txt`).

### Spelling Bee

Solves the Spelling Bee puzzle.

**Syntax:**

```bash
./solver-cli spellingbee --letters <letters> [OPTIONS]
```

**Options:**

  * `--letters <letters>`: **(Required)** Letters for the puzzle, minimum 3, duplicates allowed. First letter is special.
  * `--exclude-uncommon-words <bool>`: Exclude uncommon words from results. (Default: `false`).
  * `--must-include-first-letter <bool>`: Require words to include the first letter. (Default: `true`).
  * `--reuse-letters <bool>`: Allow unlimited reuse of letters. (Default: `true`).
  * `-o`, `--output <file>`: Specifies the output file path. (Default: `results/temp.txt`).

### Wordle

Provides entropy-based suggestions for Wordle puzzles.

**Syntax:**

```bash
./solver-cli wordle [OPTIONS]
```

**Options:**

  * `--guesses <guesses>`: A string of guess/feedback pairs separated by semicolons.
      * **Format:** `"WORD1 FFFFF;WORD2 FFFFF"` where `F` is the feedback code.
      * **Feedback Codes:** `0`=grey, `1`=yellow, `2`=green.
  * `--word-length <n>`: Word length for the puzzle. (Default: `5`, Range: `1-32`).
  * `--max-depth <0-2>`: The search depth for calculating entropy. Higher values are more accurate but slower. (Default: `0`).
  * `--exclude-uncommon-words <bool>`: If enabled, excludes less common words from the possible answers list. (Default: `false`).
  * `-o`, `--output <file>`: Specifies the output file for possible words and all guesses. (Default: `results/guesses.txt`).

### Mastermind

Provides entropy-based suggestions for Mastermind puzzles.

**Syntax:**

```bash
./solver-cli mastermind [OPTIONS]
```

**Options:**

  * `--guesses <guesses>`: A string of guess/feedback pairs separated by semicolons.
      * **Format:** `"RGBC 1 2;MYRC 1 2"` where `RGBC` is the pattern, `1` is black pegs (correct color, correct position), and `2` is white pegs (correct color, wrong position).
  * `--pegs <n>`: The number of pegs in the code. (Default: `4`, Range: `1-20`).
  * `--colors <chars>`: Available color characters as a string. (Default: `"RGBCMY"`).
  * `--allow-duplicates <bool>`: Whether to allow duplicate colors in the code. (Default: `true`).
  * `--max-depth <0-2>`: The search depth for calculating entropy. Higher values are more accurate but slower. (Default: `1`, Range: `0-2`).
  * `-o`, `--output <file>`: Specifies the output file for possible patterns and all guesses. (Default: `results/guesses.txt`).

### Dungleon

Provides entropy-based suggestions for Dungleon puzzles.

**Syntax:**

```bash
./solver-cli dungleon [OPTIONS]
```

**Options:**

  * `--guesses <guesses>`: A string of guess/feedback pairs separated by semicolons.
      * **Format:** `"ar kn ma bt dr 01234"` where character pairs are followed by color feedback.
      * **Color Codes:** `0`=not present, `1`=different position no more, `2`=correct position no more, `3`=different position one more, `4`=correct position one more.
  * `--solutions <solutions>`: Past solutions for Gauntlet mode, separated by semicolons.
      * **Format:** `"ar kn ma bt dr;cl wa ro th pr"` - just the character pairs without feedback.
  * `--max-depth <0-2>`: The search depth for calculating entropy. Higher values are more accurate but slower. (Default: `0`, Range: `0-2`).
  * `--exclude-impossible <bool>`: Exclude impossible patterns from guesses. (Default: `false`).
  * `-o`, `--output <file>`: Specifies the output file for possible patterns and all guesses. (Default: `results/dungleon.txt`).

### Read Mode

Reads and displays results from a previously generated file.

**Syntax:**

```bash
./solver-cli read [FILE] [OPTIONS]
```

**Options:**

  * `FILE`: The input file to read from. (Default: `results/temp.txt`).
  * `--start <n>`: The starting line index to display. (Default: `0`).
  * `--end <n>`: The ending line index to display. (Default: `all`).

-----

## Benchmarking

The program includes benchmarking tools to measure performance.

### Runtime Benchmark

**Syntax:**

```bash
./solver-cli --benchmark runtime <mode> [OPTIONS]
```

  * `--iterations <n>`: Number of times to run the test. (Default: `1`).
  * `--verbose`: Enable verbose output.

### Performance Benchmark

**Syntax:**

```bash
./solver-cli --benchmark performance <mode> [OPTIONS]
```

  * **Note:** Currently only implemented for Wordle mode.
  * `--verbose`: Enable verbose output.

-----

## Boolean Values

For any option that accepts a boolean value (`<bool>`), the following inputs are recognized:

  * **True:** `1`, `y`, `Y`, `yes`, `YES`, `Yes`, `t`, `T`, `true`, `True`, `TRUE`
  * **False:** `0`, `n`, `N`, `no`, `NO`, `No`, `f`, `F`, `false`, `False`, `FALSE`

-----

## Examples

**1. Solve a Letter Boxed puzzle with a fast preset and save to a specific file:**

```bash
./solver-cli letterboxed --letters abcdefghijkl --preset 2 -o results/solutions.txt
```

**1b. Use a preset but override specific settings:**

```bash
./solver-cli letterboxed --letters abcdefghijkl --preset 2 --max-depth 3 --min-word-length 3
```

**2. Solve a Spelling Bee puzzle with all optional parameters:**

```bash
./solver-cli spellingbee --letters abcdefg --exclude-uncommon-words false --must-include-first-letter true --reuse-letters true -o results/spellingbee.txt
```

**3. Get Wordle suggestions for 6-letter words with previous guesses:**

```bash
./solver-cli wordle --word-length 6 --guesses "STRAFE 010200;COINED 102010" --max-depth 1 --exclude-uncommon-words false -o results/wordle.txt
```

**4. Solve a Mastermind puzzle with custom configuration:**

```bash
./solver-cli mastermind --guesses "RGBC 1 2;MYRC 1 2" --pegs 4 --colors "RGBCMY" --allow-duplicates true --max-depth 1 -o results/mastermind.txt
```

**5. Get Dungleon suggestions with previous guesses and solutions:**

```bash
./solver-cli dungleon --guesses "ar kn bo ne fr 00010" --solutions "vi zo ne sk bt" --max-depth 0 --exclude-impossible false -o results/dungleon.txt
```

**6. Start the interactive CLI mode:**

```bash
./solver-cli -i
```

**7. Read the first 10 results from a saved file:**

```bash
./solver-cli read results/wordle.txt --start 0 --end 10
```