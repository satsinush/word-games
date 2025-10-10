cmake_minimum_required(VERSION 3.16)
project(word-games LANGUAGES CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep debug symbols by default when not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Put binaries and objects under the top-level build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Collect sources recursively from src/
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.cxx ${CMAKE_SOURCE_DIR}/src/*.cc)

add_executable(word-games ${SRCS})

target_include_directories(word-games PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Mirror Makefile warning flags
if(MSVC)
  target_compile_options(word-games PRIVATE /W4 /permissive-)
else()
  target_compile_options(word-games PRIVATE -g -Wall -Wextra -fdiagnostics-color=always)
endif()

# Friendly default output name (match previous Makefile path: build/word-games.exe)
set_target_properties(word-games PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
