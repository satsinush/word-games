# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -g -std=c++20 -Wall -Wextra -fdiagnostics-color=always

# Directory for build files
BUILD_DIR = build

# The final executable name
TARGET = $(BUILD_DIR)/word-games.exe

# Define a recursive wildcard function
rwildcard = $(shell find $(1) -name "$(2)")

# If on Windows, use a different approach that handles spaces in paths
ifeq ($(OS),Windows_NT)
    # Use PowerShell to get relative paths properly
    rwildcard = $(shell powershell -Command "Get-ChildItem -Path '$(1)' -Filter '$(2)' -Recurse | ForEach-Object { $$_.FullName.Replace('$(shell powershell -Command "(Get-Location).Path")\', '') } | ForEach-Object { $$_.Replace('\', '/') }")
endif

# --- Source Files ---
SRCS := $(call rwildcard,src,*.cpp)

# Create a list of object files in the BUILD_DIR
OBJS = $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# --- Cross-Platform Commands ---
# Default to Unix commands
RM = rm -rf
MKDIR_P = mkdir -p

# If the OS is Windows, change to Windows-native commands
ifeq ($(OS),Windows_NT)
	RM = rmdir /s /q
	MKDIR_P = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
endif

# --- Targets ---

# The default target
.PHONY: all
all: $(TARGET)

# Rule to link all object files into the final executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to create the build directory
$(BUILD_DIR):
	@$(MKDIR_P)

# Pattern rule to compile a .cpp file from 'src' into an object file in 'build'
$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
ifeq ($(OS),Windows_NT)
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
else
	@mkdir -p $(dir $@)
endif
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Target to clean up the build files
.PHONY: clean
clean:
	-$(RM) $(BUILD_DIR)