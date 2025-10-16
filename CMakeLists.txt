cmake_minimum_required(VERSION 3.16)
project(word-games LANGUAGES CXX)

# ===================================================================
# 1. PROJECT-WIDE SETTINGS
# ===================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_GUI "Build the application with the Qt GUI" ON)
option(BUILD_TESTING "Build the project's tests" ON)
option(BUILD_BENCHMARKS "Build the project's benchmarks" ON)

# ===================================================================
# 2. CREATE A BASE CORE LIBRARY (STATIC, NO DEPENDENCIES)
# ===================================================================
# This library contains only the source code and has no special dependencies.
file(GLOB_RECURSE CORE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER CORE_SRCS EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/src/gui/.*")
list(FILTER CORE_SRCS EXCLUDE REGEX ".*/main.cpp")

add_library(word-games-core-base STATIC ${CORE_SRCS})

target_include_directories(word-games-core-base PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(MSVC)
    target_compile_options(word-games-core-base PRIVATE /W4 /permissive-)
else()
    target_compile_options(word-games-core-base PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()


# ===================================================================
# 3. SETUP TRACY AND DECORATOR LIBRARIES
# ===================================================================
# This section fetches Tracy and creates two INTERFACE libraries that "wrap"
# our base library: one with profiling enabled, and one without.

include(FetchContent)
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        v0.12.2
)
FetchContent_MakeAvailable(tracy)

# **FIX:** Add the Tracy include path directly to the base library.
# The PUBLIC keyword ensures that the base library can compile its own files,
# and that any downstream target (like our executables) also gets this path.
target_include_directories(word-games-core-base PUBLIC ${tracy_SOURCE_DIR}/public)


# -- Create the NON-PROFILED interface library for tests and benchmarks --
# This simply wraps the base library. It no longer needs to specify include paths.
add_library(word-games-core INTERFACE)
target_link_libraries(word-games-core INTERFACE word-games-core-base)

# -- Create the PROFILED interface library for the main application --
# This wraps the base library and adds the profiling flags and library link.
add_library(word-games-core-profiled INTERFACE)
target_link_libraries(word-games-core-profiled INTERFACE word-games-core-base)

# Only this version gets the TRACY_ENABLE definition and links to the client library.
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(word-games-core-profiled INTERFACE TRACY_ENABLE)
    target_link_libraries(word-games-core-profiled INTERFACE TracyClient)
endif()


# ===================================================================
# 4. SETUP SHARED RESOURCES
# ===================================================================
set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${RESOURCE_DIR}
            ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying shared resources..."
)

# ===================================================================
# 5. CREATE THE GUI LIBRARY & MAIN EXECUTABLE
# ===================================================================
if(BUILD_GUI)
    message(STATUS "GUI build enabled. Searching for Qt6...")
    find_package(Qt6 COMPONENTS Widgets REQUIRED)

    file(GLOB_RECURSE GUI_SRCS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/gui/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.ui"
    )
    add_library(word-games-gui STATIC ${GUI_SRCS})
    
    set_target_properties(word-games-gui PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
    
    # **FIX:** Use a generator expression to select the correct core library.
    # Links to the profiled version in Debug, and the clean version otherwise.
    target_link_libraries(word-games-gui PUBLIC
        $<IF:$<CONFIG:Debug>,word-games-core-profiled,word-games-core>
        Qt6::Widgets
    )
    target_compile_definitions(word-games-gui PUBLIC WITH_GUI)

    add_executable(word-games src/main.cpp)
    target_link_libraries(word-games PRIVATE word-games-gui)
    set_target_properties(word-games PROPERTIES AUTOMOC ON)
    add_dependencies(word-games copy_resources)

    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${CMAKE_PREFIX_PATH}/bin)
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET word-games POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} ARGS $<TARGET_FILE:word-games>
                WORKING_DIRECTORY $<TARGET_FILE_DIR:word-games>
                COMMENT "Running windeployqt...")
        endif()
    endif()
else() # CLI-only build
    message(STATUS "GUI build disabled. Building CLI-only version.")
    add_executable(word-games src/main.cpp)
    target_link_libraries(word-games PRIVATE
        $<IF:$<CONFIG:Debug>,word-games-core-profiled,word-games-core>
    )
    add_dependencies(word-games copy_resources)
endif()


# ===================================================================
# 6. SETUP TESTING (NO GUI DEPENDENCIES)
# ===================================================================
if(BUILD_TESTING)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    add_executable(word-games-tests tests/tests.cpp)
    # **FIX:** Tests ALWAYS link to the clean, non-profiled core library.
    target_link_libraries(word-games-tests PRIVATE word-games-core gtest_main)
    add_dependencies(word-games-tests copy_resources)

    include(GoogleTest)
    if(WIN32)
        get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
        set(SYSTEM32_PATH "$ENV{SystemRoot}/System32")
        set(TEST_ENVIRONMENT_LIST "PATH=${MINGW_BIN_DIR};${SYSTEM32_PATH}")
        gtest_discover_tests(word-games-tests
            PROPERTIES
                ENVIRONMENT "${TEST_ENVIRONMENT_LIST}"
        )
    else()
        gtest_discover_tests(word-games-tests)
    endif()
else()
    message(STATUS "Testing disabled. Skipping test build.")
endif()


# ===================================================================
# 7. SETUP BENCHMARKING (NO GUI DEPENDENCIES)
# ===================================================================
if(BUILD_BENCHMARKS)
    message(STATUS "Benchmarks enabled. Building benchmark runner.")
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.4
    )
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(word-games-benchmarks benchmarks/benchmarks.cpp)
    # **FIX:** Benchmarks ALWAYS link to the clean, non-profiled core library.
    target_link_libraries(word-games-benchmarks PRIVATE word-games-core benchmark::benchmark_main)
    add_dependencies(word-games-benchmarks copy_resources)
else()
    message(STATUS "Benchmarks disabled. Skipping benchmark build.")
endif()