# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -g -std=c++20 -Wall -Wextra -fdiagnostics-color=always

# The final executable name (place executable inside the build directory)
TARGET = $(BUILD_DIR)/word-games.exe

# Directory for build files
BUILD_DIR = build

# Find all .cpp files in the 'src' directory
SRCS = $(wildcard src/*.cpp)

# Create a list of object files in the BUILD_DIR
# This substitutes 'src/%.cpp' with 'build/%.o'
OBJS = $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Command to remove files and directories
RM = rm -rf

# --- Targets ---

# The default target
.PHONY: all
all: $(TARGET)

# Rule to link all object files into the final executable
# Ensure BUILD_DIR exists before linking (order-only prerequisite)
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to create the build directory before compiling anything
# The '|' makes it an "order-only prerequisite"
$(OBJS): | $(BUILD_DIR)
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pattern rule to compile a .cpp file from 'src' into an object file in 'build'
$(BUILD_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Target to clean up the build files
.PHONY: clean
clean:
	$(RM) $(BUILD_DIR) $(TARGET)