cmake_minimum_required(VERSION 3.16)
project(word-games LANGUAGES CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep your recursive file search with the flag that makes it reliable
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
)

add_executable(word-games ${SRCS})

target_include_directories(word-games PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Let CMAKE_BUILD_TYPE handle the '-g' flag for debugging
if(MSVC)
    target_compile_options(word-games PRIVATE /W4 /permissive-)
else()
    target_compile_options(word-games PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()