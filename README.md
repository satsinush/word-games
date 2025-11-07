# Puzzle++ 🧩
Advanced word game and puzzle solver created in C++

A comprehensive C++ application that solves multiple popular word puzzles and games with both GUI and command-line interfaces. Supports interactive solving, automated batch processing, and entropy-based game assistance.

-----

## Features

  * **Multiple Game Solvers**: Letter Boxed, Spelling Bee, Wordle, Mastermind, and Dungleon
  * **Dual Interface**: Qt-based GUI and full-featured command-line interface
  * **Interactive & Automated Modes**: User-guided solving or scriptable batch processing
  * **Entropy-Based Suggestions**: Advanced algorithms for optimal move recommendations
  * **Configurable Solvers**: Multiple presets and custom options for performance tuning
  * **Output Management**: Save results to files, read previous solutions, benchmarking tools

-----

## Quick Start

### GUI Mode (Default)
```bash
./p++
```
Launch the graphical interface for interactive puzzle solving.

### Interactive CLI Mode
```bash
./p++ -i
```
Text-based interactive mode with guided prompts.

### Direct Command Examples
```bash
# Solve Letter Boxed puzzle
./p++ letterboxed --letters abcdefghijkl --preset 2

# Solve Spelling Bee with custom letters
./p++ spellingbee --letters nyhacked --reuse-letters true

# Get Wordle suggestions
./p++ wordle --guesses "CRANE 01120" --max-depth 1

# Mastermind solver assistance
./p++ mastermind --guesses "RGBC 1 2" --pegs 4 --colors "RGBCMY"
```

## Complete Documentation

For comprehensive usage instructions, all command-line options, game modes, and detailed examples, see:

**📖 [Complete Usage Guide](docs/usage.md)**

The usage guide covers:
- All supported game modes and solvers
- Complete command-line reference
- Advanced configuration options
- Benchmarking and performance tools
- Boolean value formats and examples

-----

## Building the Project

### Requirements
- **C++ Compiler**: Supports C++20 standard (GCC 10+, Clang 10+, MSVC 2019+)
- **CMake**: Version 3.16 or later
- **Qt Framework**: Version 6.0+ (for GUI support, optional)

### Build Instructions

1. **Clone and navigate:**
   ```bash
   git clone <repository-url>
   cd word-games
   ```

2. **Configure and build:**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **Optional GUI support:**
   ```bash
   cmake -DWITH_GUI=ON ..
   cmake --build . --config Release
   ```

The compiled executable will be named `p++` (or `p++.exe` on Windows).

## Licensing

The source code for this project is licensed under the **MIT License**. See the `LICENSE` file for details.

### Third-Party Dependencies

This project uses several third-party libraries with their respective licenses:

- **Qt Framework**: **LGPLv3 License** - See `LICENSE-QT` for full license text. Qt source code is available at the [official Qt website](https://www.qt.io/download-open-source). To enable relinking against a modified Qt library, the full source code for this application and its build scripts (CMake files) are provided here.

- **Tracy Profiler**: **3-Clause BSD License** - Used for optional performance profiling support (development dependency).

- **Google Test**: **3-Clause BSD License** - Used for unit testing framework (development dependency).

- **Google Benchmark**: **Apache License 2.0** - Used for performance benchmarking (development dependency).

All third-party dependencies are automatically fetched during the CMake build process and do not need to be installed separately.

### Game Assets

- **Dungleon Images**: The character and item images in `resources/dungleon/` are sourced from the official [Dungleon website](https://www.dungleon.com/) and are used in this puzzle solver implementation.