# Word Games Solver 🧩

This is a C++ command-line tool designed to solve two popular word puzzles: **New York Times Letter Boxed** and **Spelling Bee**. It can be run interactively or by using command-line arguments for automated solving.

-----

## Features

  * **Letter Boxed Solver**: Finds words and solutions to the Letter Boxed puzzle.
  * **Spelling Bee Solver**: Finds all valid words and pangrams for the Spelling Bee puzzle.
  * **Interactive Mode**: Guides you through entering puzzle letters and solver options.
  * **Command-Line Mode**: Allows you to solve puzzles directly from the terminal with specified arguments, which is great for scripting or automated tasks.
  * **Configurable Solver**: The Letter Boxed solver offers different presets (Fast, Default, Thorough) and custom options to balance speed and completeness.
  * **Output Management**: Solutions can be saved to a file for later viewing.

-----

## How to Use

### Interactive Mode

To run the program in interactive mode, simply execute the compiled binary without any arguments.

```bash
./word_solver
```

The program will then prompt you to select a game mode and enter the puzzle's letters.

### Command-Line Mode

You can also solve puzzles by providing command-line arguments. This is useful for quickly getting results without going through the interactive prompts.

#### Letter Boxed

To solve a Letter Boxed puzzle, specify the letters and a preset.

```bash
./word_solver --mode letterboxed --letters abcdefghijkl --preset 1
```

  * `--mode letterboxed`: Specifies the game mode.
  * `--letters <12letters>`: The 12 letters on the puzzle.
  * `--preset <1|2|3|0>`: The solver preset.
      * **1**: Default (balanced, finds solutions up to 2 words)
      * **2**: Fast (less thorough, quick results)
      * **3**: Thorough (exhaustive, finds solutions up to 3 words)
      * **0**: Custom (requires additional arguments for full control)

You can override preset settings or use a custom configuration (`--preset 0`) by providing the following optional arguments:

  * `--maxDepth <num>`: Max number of words per solution (e.g., `2` for two-word solutions).
  * `--minWordLength <num>`: Minimum length of a word (e.g., `3`).
  * `--minUniqueLetters <num>`: Minimum number of unique letters a word must contain.
  * `--pruneRedundantPaths <0|1>`: `1` to enable, `0` to disable.
  * `--pruneDominatedClasses <0|1>`: `1` to enable, `0` to disable.
  * `--file <filename>`: Path to a file to save the solutions.
  * `--maxResults <num>`: Maximum number of solutions to display.

#### Spelling Bee

To solve a Spelling Bee puzzle, provide the 7 letters.

```bash
./word_solver --mode spellingbee --letters abcdefg
```

  * `--mode spellingbee`: Specifies the game mode.
  * `--letters <7letters>`: The 7 letters for the puzzle. The first letter you enter is the center letter.
  * `--file <filename>`: Path to a file to save the solutions.
  * `--maxResults <num>`: Maximum number of solutions to display.

#### Read Solutions from File

You can view saved solutions from a file without rerunning the solver.

```bash
./word_solver --mode read --file solutions.txt --start 0 --end 10
```

  * `--mode read`: Specifies read mode.
  * `--file <filename>`: The file to read from.
  * `--start <startIndex>`: The starting index of the solutions to display.
  * `--end <endIndex>`: The ending index of the solutions to display.

-----

## Building the Project

The project is written in C++ and uses standard libraries. You'll need a C++ compiler (like g++ or Clang) that supports C++17 or later.

1.  Clone the repository.
2.  Navigate to the project directory.
3.  Compile the source files.

<!-- end list -->

```bash
g++ main.cpp utils.cpp letterBoxed.cpp spellingBee.cpp -o word_solver -std=c++17 -Wall -Wextra
```

You may need to include additional header and source files if the project is structured differently.
The compiled executable will be named `word_solver`.