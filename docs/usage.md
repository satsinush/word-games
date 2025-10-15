# Word Game Solver CLI Usage

This document outlines the usage, options, and modes for the Word Game Solver command-line interface.

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

The solver supports several game modes, each with its own set of specific options.

### Letter Boxed

Solves the Letter Boxed puzzle.

**Syntax:**

```bash
./solver-cli letterboxed --letters <12letters> [OPTIONS]
```

**Options:**

  * `--letters <letters>`: **(Required)** The 12 letters for the puzzle, provided as a single string (e.g., `abcdefghijkl`).
  * `--preset <1|2|3|0>`: Specifies a preset configuration for the solver.
      * `1`: Default - A balanced approach.
      * `2`: Fast - Prioritizes speed over finding every possible solution.
      * `3`: Thorough - A comprehensive search that may take longer.
      * `0`: Custom - Allows for manual configuration using the options below.
  * `--max-depth <n>`: The maximum number of words allowed in a single solution. (Required if `preset=0`).
  * `--min-word-length <n>`: The minimum length for a valid word. (Required if `preset=0`).
  * `--min-unique-letters <n>`: The minimum number of unique letters a word must contain. (Required if `preset=0`).
  * `--prune-paths <bool>`: **(Recommended)** Stops processing paths if a word is added that doesn't cover any new letters. This is always done on words that start and end on the same letter but may exclude very specific situations where a short word is needed to switch starting letters.
  * `--prune-classes <bool>`: **(Recommended)** Eliminates classes of words that have fewer unique letters. This may exclude solutions that use more common words in favor of using word classes that cover more letters, even if those words are less common.
  * `-o`, `--output <file>`: Specifies the output file path. (Default: `results/temp.txt`).

### Spelling Bee

Solves the Spelling Bee puzzle.

**Syntax:**

```bash
./solver-cli spellingbee --letters <7letters> [OPTIONS]
```

**Options:**

  * `--letters <letters>`: **(Required)** The 7 letters for the puzzle, with the center letter first.
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
  * `--max-depth <0-2>`: The search depth for calculating entropy. Higher values are more accurate but slower. (Default: `0`).
  * `--exclude-uncommon-words <bool>`: If enabled, excludes less common words from the possible answers list.
  * `-o`, `--output <file>`: Specifies the output file for possible words and all guesses. (Default: `results/guesses.txt`).

### Mastermind

Provides entropy-based suggestions for Mastermind puzzles.

**Syntax:**

```bash
./solver-cli mastermind [OPTIONS]
```

**Options:**

  * `--guesses <guesses>`: A string of guess/feedback pairs separated by semicolons.
      * **Format:** `"P P P P|B W;P P P P|B W"` where `P` is the peg color, `B` is black pegs (correct color, correct position), and `W` is white pegs (correct color, wrong position).
  * `--num-pegs <n>`: The number of pegs in the code. (Default: `4`).
  * `--num-colors <n>`: The number of available colors. (Default: `6`).
  * `--allow-duplicates <bool>`: Whether to allow duplicate colors in the code. (Default: `true`).
  * `--max-depth <1-3>`: The search depth for calculating entropy. Higher values are more accurate but slower. (Default: `1`).
  * `-o`, `--output <file>`: Specifies the output file for possible patterns and all guesses. (Default: `results/guesses.txt`).

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

**2. Start the interactive CLI mode:**

```bash
./solver-cli -i
```

**3. Get Wordle suggestions based on two previous guesses and save the results:**

```bash
./solver-cli wordle --guesses "STEAL 01201;CRANE 00120" --max-depth 1 -o results/wordle.txt
```

**4. Read the first 10 results from a saved Wordle file:**

```bash
./solver-cli read results/wordle.txt --start 0 --end 10
```