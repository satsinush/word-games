# Puzzle++ 🧩

**Advanced word game and puzzle solver created in C++**

A comprehensive C++ application that solves multiple popular word puzzles and games with both GUI and command-line interfaces. Supports interactive solving, automated batch processing, and entropy-based game assistance.

-----

## Features

* **Multiple Game Solvers**: Letter Boxed, Spelling Bee, Wordle, Mastermind, Hangman, and Dungleon
* **Dual Interface**: Qt-based GUI and full-featured command-line interface
* **Interactive & Automated Modes**: User-guided solving or scriptable batch processing
* **Entropy-Based Suggestions**: Advanced algorithms for optimal move recommendations
* **Configurable Solvers**: Multiple presets and custom options for performance tuning
* **Output Management**: Save results to files, read previous solutions, benchmarking tools

-----

## Installation & Building

### 1. Windows GUI (Recommended)

For the best Windows experience, use the Installer from the latest GitHub Release:

[Download the latest installer](https://github.com/satsinush/word-games/releases/latest)

This installer bundles the GUI and required runtimes so no separate Qt or compiler setup is needed.

* **No Setup Required:** No need to install Qt or compilers.
* **Easy Access:** The installer creates an application shortcut in your Start Menu which can be used to launch the main application with the GUI.
* **Uninstall:** Easily removable via Windows "Add or Remove programs".

### 2. Building from Source

If you are on Linux/macOS, or prefer to build from source on Windows, follow the instructions below.

**Requirements:**

* **C++ Compiler**: Supports C++20 (GCC 10+, Clang 10+, MSVC 2019+)
* **CMake**: Version 3.16 or later
* **Qt Framework**: Version 6.0+ (Only required if building the GUI)

#### Preparation
```bash
git clone <repository-url>
cd word-games
```

#### Option A: Core Build Only (CLI)

For a lightweight build with **no GUI dependency** (command-line interface only), you can build using the core preset. This requires only a C++ compiler and CMake.

```bash
# Build using the core preset (No Qt required)
cmake --preset mingw-core
cmake --build --preset mingw-core

# Run the application (Interactive Mode)
./build/mingw-core/p++
```

*Note: Since there is no GUI, running the executable directly enters interactive CLI mode.*

#### Option B: Full Build (GUI + CLI)

To build the full application with the GUI, you must have Qt 6 installed.

*Note: You may need to adjust the CMake presets (`CMakePresets.json`) to point to your specific Qt installation paths.*

```bash
# Build using the release preset (Requires Qt)
cmake --preset mingw-release
cmake --build --preset mingw-release

# Run the application
./build/mingw-release/p++
```

#### Option C: Building Windows Installer (NSIS)

To build a standalone Windows `.exe` installer (containing `p++.exe`, Qt 6 DLLs deployed via `windeployqt`, and resources):

**Prerequisites**:
1. **Qt 6**: Installed on your build environment (e.g. `C:\Qt\6.x.x\msvc2019_64` or MinGW).
2. **NSIS (Nullsoft Scriptable Install System)**: Installed and added to system `PATH`.

```powershell
# 1. Configure release build using the release preset
cmake --preset mingw-release

# 2. Build the application and run windeployqt
cmake --build --preset mingw-release

# 3. Build the NSIS installer package via CPack
cpack --config build/mingw-release/CPackConfig.cmake

# Or use the custom CMake target:
cmake --build --preset mingw-release --target package_release_installer
```

The output installer binary will be generated in `build/mingw-release/Puzzle++-3.0.0-win64.exe`.

-----

## Quick Start

### Interactive Mode

If you installed via the Windows Installer, simply launch **Puzzle++** from your Start Menu.

If using the CLI, run the executable directly:

```bash
./p++ -i
```

### Direct Command Examples

```bash
# Solve Letter Boxed puzzle
./p++ letterboxed --letters abcdefghijkl --preset 2

# Solve Spelling Bee with custom letters
./p++ spellingbee --letters nyhacked --reuse-letters true

# Get Wordle suggestions using auto-depth
./p++ wordle --guesses "CRANE 01120" --auto-depth

# Mastermind solver assistance
./p++ mastermind --guesses "RGBC 1 2" --pegs 4 --colors "RGBCMY"

# Hangman letter suggestions
./p++ hangman --input "_A__ ___;etz"
```

## Running Tests

To run the unit tests, make sure you configure the build using a preset that has testing enabled (e.g. `linux-debug` or `mingw-debug`), build the project, and execute the test suite:

```bash
# Run the tests binary directly
./build/linux-debug/p++-tests

# Or run using CTest
ctest --test-dir build/linux-debug
```

## Running Benchmarks

Benchmarks are built as a separate `p++-benchmarks` binary when `BUILD_BENCHMARKS` is enabled. Debug presets such as `linux-debug`, `mingw-debug`, and `msvc-debug` turn this on by default.

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

## Git Hooks (Pre-push Validation)

This repository includes a pre-push hook script in `.githooks/pre-push` that runs `clang-format` formatting checks and `ctest` unit tests.

To enable the git hook locally, run:

```bash
git config core.hooksPath .githooks
```

## Complete Documentation

For GUI walkthroughs, full CLI options, and examples, see **[docs/usage.md](./docs/usage.md)**.

-----

## Licensing

The source code for this project is licensed under the **MIT License**. See the `LICENSE` file for details.

### Third-Party Dependencies

This project uses several third-party libraries with their respective licenses:

  * **Qt Framework**: **LGPLv3 License** - See `LICENSE-QT` for full license text. Qt source code is available at the [official Qt website](https://www.qt.io/download-open-source). To enable relinking against a modified Qt library, the full source code for this application and its build scripts (CMake files) are provided here.
  * **Tracy Profiler**: **3-Clause BSD License** - Used for optional performance profiling support (development dependency).
  * **Google Test**: **3-Clause BSD License** - Used for unit testing framework (development dependency).
  * **Google Benchmark**: **Apache License 2.0** - Used for performance benchmarking (development dependency).

All third-party dependencies are automatically fetched during the CMake build process and do not need to be installed separately.

### Game Assets

  * **Dungleon Images**: The character and item images in `resources/dungleon_characters/` are sourced from the official [Dungleon website](https://www.dungleon.com/) and are used in this puzzle solver implementation.