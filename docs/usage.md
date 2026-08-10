# Puzzle++ Usage Guide

Puzzle++ (`p++`) solves Wordle, Spelling Bee, Letter Boxed, Mastermind, Hangman, and Dungleon. It ships as a Qt GUI and a CLI. In this homelab, the dashboard Word Games UI calls the `p++` CLI binary (compiled inside the API container).

## Launching

1. **GUI** — run `p++` with no arguments (requires a GUI build).
2. **Interactive CLI** — `./p++ -i`
3. **Direct command** — `./p++ <mode> [options]`

---

# Part 1: GUI

Screenshots are from the desktop Qt app. Controls mirror the CLI options for each game.

## Wordle

![Wordle screenshot](./screenshots/wordle.png)

1. **Configuration (⚙️):** word length (default 5); search depth `0` / `1` / `2+`; optionally exclude uncommon words.
2. **Playing:** enter a guess, cycle letter colors (gray / yellow / green), then **Solve**.
3. **Results:** ENT (expected turns; lower is better), word score, probability.

## Spelling Bee

![Spelling Bee screenshot](./screenshots/spellingbee.png)

1. **Configuration:** exclude uncommon words; must-include center letter; letter reuse (off = anagram mode).
2. **Playing:** enter letters; **first letter is the center**. The table lists valid words.

## Letter Boxed

![Letter Boxed screenshot](./screenshots/letterboxed.png)

1. **Configuration:** preset (`Default` / `Fast` / `Thorough`), max words, pruning.
2. **Playing:** enter 12 letters as one string, **Enter** to solve.

## Mastermind

![Mastermind screenshot](./screenshots/mastermind.png)

1. **Configuration:** peg count; color characters (default `RGBCMY`).
2. **Playing:** enter a code guess, set black/white peg counts, **Solve**.

## Hangman

1. **Configuration:** search depth; exclude uncommon words.
2. **Playing:** pattern with `_` for unknowns (spaces separate multi-word phrases); mark letters as in-word or strikes; **Solve**.
3. **Results:** suggested letter, ENT, in-word probability, sample matching words.

## Dungleon

![Dungleon screenshot](./screenshots/dungleon.png)

1. Click character icons to build a guess; click characters in the guess to cycle feedback states (red / yellow / green, with “+” variants for duplicate counts).
2. **Gauntlet:** **Submit Solution** for past solutions; **Submit Guess** for feedback rows.
3. **Solve** for entropy-ranked suggestions.

---

# Part 2: CLI

```bash
./p++ [OPTIONS] [MODE]
```

### Global options

* `-h`, `--help` — help
* `-v`, `--verbose` — verbose logging (reserved)
* `-i` — interactive CLI

### Boolean values

For `<bool>` options: `1` / `y` / `yes` / `t` / `true` (any case) or `0` / `n` / `no` / `f` / `false`.

---

## Game modes

### Wordle

```bash
./p++ wordle [OPTIONS]
```

* `--guesses <string>` — `WORD FEEDBACK` pairs separated by `;`. Feedback: `0` grey, `1` yellow, `2` green. Example: `"AUDIO 00100;SOARE 10201"`
* `--word-length <n>` — default `5`
* `--max-depth <0-2>` — entropy depth (default `0`)
* `--auto-depth` — overrides `--max-depth`
* `--exclude-uncommon-words <bool>` — default `false`
* `-o`, `--output <file>` — default `results/guesses.txt`

### Spelling Bee

```bash
./p++ spellingbee --letters <letters> [OPTIONS]
```

* `--letters <string>` — **required**, ≥3 letters; **first is center**
* `--exclude-uncommon-words <bool>` — default `false`
* `--must-include-first-letter <bool>` — default `true`
* `--reuse-letters <bool>` — default `true`
* `-o`, `--output <file>` — default `results/temp.txt`

### Letter Boxed

```bash
./p++ letterboxed --letters <12letters> [OPTIONS]
```

* `--letters <string>` — **required**, 12 letters (e.g. `abcdefghijkl`)
* `--preset <0-3>` — `1` Default, `2` Fast, `3` Thorough, `0` Custom
* `--max-depth <n>` — max words in a solution
* `--min-word-length <n>`
* `--min-unique-letters <n>`
* `--prune-paths <bool>` — recommended
* `--prune-classes <bool>` — recommended
* `-o`, `--output <file>` — default `results/temp.txt`

### Mastermind

```bash
./p++ mastermind [OPTIONS]
```

* `--guesses <string>` — `CODE BLACK WHITE` pairs; e.g. `"RGBC 1 2"`
* `--pegs <n>` — default `4`
* `--colors <string>` — default `"RGBCMY"`
* `--allow-duplicates <bool>` — default `true`
* `--max-depth <0-2>` — default `1`
* `--auto-depth`
* `-o`, `--output <file>` — default `results/guesses.txt`

### Hangman

```bash
./p++ hangman [OPTIONS]
```

* `--input <string>` — `"PATTERN;STRIKES"` (e.g. `"_A__ ___;etxzq"`)
* `--pattern <string>` / `--strikes <string>` — alternatives to `--input`
* `--max-depth <0-2>` — default `0`
* `--auto-depth`
* `--exclude-uncommon-words <bool>` — default `false`
* `-o`, `--output <file>` — default `results/hangman.txt`

### Dungleon

```bash
./p++ dungleon [OPTIONS]
```

* `--guesses <string>` — character pairs + feedback; e.g. `"ar kn ma bt dr 01234"`
  * Codes: `0` absent, `1` wrong pos (no more), `2` correct pos (no more), `3` wrong pos (+more), `4` correct pos (+more)
* `--solutions <string>` — past Gauntlet solutions, `;`-separated
* `--max-depth <0-2>` — default `0`
* `--auto-depth`
* `--exclude-impossible <bool>` — default `false`
* `-o`, `--output <file>` — default `results/dungleon.txt`

### Read mode

```bash
./p++ read [FILE] [OPTIONS]
```

* `--start <n>` / `--end <n>` — line range (defaults: `0` … end)

---

## Benchmarking

Built as `p++-benchmarks` when `BUILD_BENCHMARKS` is on (debug presets enable this).

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/p++-benchmarks
./build/linux-debug/p++-benchmarks --benchmark_time_unit=ms
./build/linux-debug/p++-benchmarks --benchmark_filter=BM_Wordle
```

MinGW: `./build/mingw-debug/p++-benchmarks.exe`. Covered: Wordle, Spelling Bee, Letter Boxed, Mastermind, Dungleon, Hangman.

---

## CLI examples

```bash
./p++ wordle --word-length 5 --max-depth 1 --exclude-uncommon-words true --guesses "STEAL 20100;CRANE 01002" -o results/wordle.txt
./p++ spellingbee --letters nhmkace --must-include-first-letter true --reuse-letters true -o results/spellingbee.txt
./p++ letterboxed --letters uvjswitgebac --preset 0 --max-depth 3 --min-word-length 4 --prune-paths true -o results/letterboxed.txt
./p++ mastermind --guesses "RGBC 1 2;MYRC 1 2" --pegs 4 --colors "RGBCMY" --max-depth 1 -o results/mastermind.txt
./p++ hangman --input "_A___ ___;etz" --max-depth 1 --exclude-uncommon-words true -o results/hangman.txt
./p++ dungleon --guesses "ar kn bo ne fr 00010" --solutions "vi zo ne sk bt" --max-depth 0 -o results/dungleon.txt
```
