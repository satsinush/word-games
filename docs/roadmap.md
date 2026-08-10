# Puzzle++ Roadmap, Known Issues, and Limitations

This document tracks planned features, performance enhancements, and known issues for the **Puzzle++ (`p++`)** C++ solver and Qt GUI application.

---

## 🗺️ Feature Roadmap & Expansion Ideas

### Additional Games Support
* **Waffle**: Grid-based letter swap solver for daily Waffle puzzles.
* **Strands**: Theme-based word search matrix solver using trie dictionary lookup.
* **Connections**: Word group categorization solver based on semantic similarity.

### Profiling & Diagnostic Enhancements
* **Tracy Profiler Integration**: Resolve debug heap allocation handling when compiling with `-DBUILD_TRACY=ON` to enable memory and frame-time tracing.
* **Mobile / Standalone Packages**: Package Qt 6 GUI using Qt for Android to generate standalone Android `.apk` release packages.

---

## 🐛 Known Issues

* **Qt 6 Deployment Dependency**: Generating the Windows NSIS installer requires NSIS and `windeployqt` installed on the host machine.
* **Large Dictionary Memory Footprint**: Loading full 6-letter to 12-letter dictionaries into memory requires ~50MB RAM during heavy ENT tree exploration.
