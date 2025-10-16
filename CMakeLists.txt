cmake_minimum_required(VERSION 3.16)
project(word-games LANGUAGES CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_GUI "Build the application with the Qt GUI" ON)
option(BUILD_TESTING "Build the project's tests" ON)
option(BUILD_BENCHMARKS "Build the project's benchmarks" ON)


# ===================================================================
# 1. CREATE A LIBRARY FOR YOUR CORE CODE
# ===================================================================
# By putting your game logic into a library, you can link it into
# both your main executable AND your test executable.

# Glob for source files, excluding the gui directory and the main entry point
file(GLOB_RECURSE CORE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER CORE_SRCS EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/src/gui/.*")
# Exclude main.cpp from the library, as it will be in the executable
list(FILTER CORE_SRCS EXCLUDE REGEX ".*/main.cpp")

if(BUILD_GUI)
    message(STATUS "GUI build enabled. Searching for Qt6...")
    find_package(Qt6 COMPONENTS Widgets REQUIRED)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    file(GLOB_RECURSE GUI_SRCS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/gui/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.ui"
    )
    list(APPEND CORE_SRCS ${GUI_SRCS})
else()
    message(STATUS "GUI build disabled. Building CLI-only version.")
endif()

# Create the library named 'word-games-lib'
add_library(word-games-lib ${CORE_SRCS})

# Apply settings to the library
target_include_directories(word-games-lib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(MSVC)
    target_compile_options(word-games-lib PRIVATE /W4 /permissive-)
else()
    target_compile_options(word-games-lib PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()

if(BUILD_GUI)
    target_compile_definitions(word-games-lib PUBLIC WITH_GUI)
    target_link_libraries(word-games-lib PUBLIC Qt6::Widgets)
endif()


# ===================================================================
# 2. CREATE THE MAIN EXECUTABLE
# ===================================================================
# The executable now only needs main.cpp and links to our library.
add_executable(word-games src/main.cpp)
target_link_libraries(word-games PRIVATE word-games-lib)

# Copy resources and deploy Qt DLLs for the main executable
# (This part is unchanged)
set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
add_custom_command(
    TARGET word-games POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RESOURCE_DIR}
        $<TARGET_FILE_DIR:word-games>/resources
    COMMENT "Copying resources to build directory..."
)
if(WIN32 AND BUILD_GUI)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${CMAKE_PREFIX_PATH}/bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(
            TARGET word-games POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} ARGS $<TARGET_FILE:word-games>
            WORKING_DIRECTORY $<TARGET_FILE_DIR:word-games>
            COMMENT "Running windeployqt..."
        )
    endif()
endif()


# ===================================================================
# 3. SETUP TESTING
# ===================================================================
if(BUILD_TESTING)
    # Enable testing for the project
    enable_testing()

    # This will download and configure Google Test for our project.
    include(FetchContent)
    FetchContent_Declare(
      googletest
      SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest-1.17.0"
    )
    # For Windows: Prevent gtest from installing itself globally
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Create the test executable
    add_executable(word-games-tests tests/tests.cpp)

    # Link the test executable against our code library and Google Test
    target_link_libraries(word-games-tests PRIVATE word-games-lib gtest_main)

    # Automatically discover and add all tests from the executable to CTest
    include(GoogleTest)
    gtest_discover_tests(word-games-tests)
else()
    message(STATUS "Testing disabled. Skipping test build.")
endif()

# ===================================================================
# 4. SETUP BENCHMARKING
# ===================================================================
if(BUILD_BENCHMARKS)
    message(STATUS "Benchmarks enabled. Building benchmark runner.")

    # FetchContent is already included in your testing block
    include(FetchContent)
    
    # Disable building benchmark tests to avoid compilation errors
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
    
    FetchContent_Declare(
        googlebenchmark
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/benchmark-1.9.4"
    )
    FetchContent_MakeAvailable(googlebenchmark)

    # Create the benchmark executable.
    # Assumes your benchmark source files are in a 'benchmarks/' directory.
    add_executable(word-games-benchmarks benchmarks/benchmarks.cpp)

    # Link against your core library and Google Benchmark's main library.
    target_link_libraries(word-games-benchmarks PRIVATE word-games-lib benchmark::benchmark_main)
else()
    message(STATUS "Benchmarks disabled. Skipping benchmark build.")
endif()