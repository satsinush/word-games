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
option(WITH_TRACY "Enable Tracy profiling support" ON)

# ===================================================================
# 2. CREATE A CORE OBJECT LIBRARY
# ===================================================================
file(GLOB_RECURSE CORE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER CORE_SRCS EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/src/gui/.*")
list(FILTER CORE_SRCS EXCLUDE REGEX ".*/main.cpp")

add_library(word-games-core-objects OBJECT ${CORE_SRCS})

target_include_directories(word-games-core-objects PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(MSVC)
    target_compile_options(word-games-core-objects PRIVATE /W4 /permissive-)
else()
    target_compile_options(word-games-core-objects PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()

# ===================================================================
# 3. SETUP SHARED RESOURCES
# ===================================================================
set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${RESOURCE_DIR}
            ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying shared resources..."
)

# ===================================================================
# 4. SETUP TRACY AND CREATE LIBRARY VARIANTS
# ===================================================================
# --- Create the standard, non-profiled library from the objects ---
add_library(word-games-core-base STATIC $<TARGET_OBJECTS:word-games-core-objects>)

# --- Conditionally fetch and set up Tracy ---
if(WITH_TRACY)
    message(STATUS "Tracy profiling enabled.")
    include(FetchContent)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG        v0.12.2
    )
    FetchContent_MakeAvailable(tracy)

    # Add Tracy's include path only when it's enabled and has been fetched
    target_include_directories(word-games-core-objects PUBLIC ${tracy_SOURCE_DIR}/public)

    # Create the profiled library variant
    add_library(word-games-core-profiled STATIC $<TARGET_OBJECTS:word-games-core-objects>)
    target_compile_definitions(word-games-core-profiled PRIVATE TRACY_ENABLE)
    target_link_libraries(word-games-core-profiled INTERFACE TracyClient)
else()
    message(STATUS "Tracy profiling disabled.")
endif()

# ===================================================================
# 5. CREATE THE MAIN EXECUTABLE (WITH OPTIONAL GUI)
# ===================================================================
# -- Conditionally create the GUI static library --
if(BUILD_GUI)
    message(STATUS "GUI build enabled.")
    find_package(Qt6 COMPONENTS Widgets REQUIRED)

    file(GLOB_RECURSE GUI_SRCS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/gui/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.ui"
    )
    add_library(word-games-gui STATIC ${GUI_SRCS})
    
    target_include_directories(word-games-gui PUBLIC ${CMAKE_SOURCE_DIR}/src)
    set_target_properties(word-games-gui PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
    
    # Link the GUI library to its dependencies
    target_link_libraries(word-games-gui PUBLIC Qt6::Widgets)
    target_compile_definitions(word-games-gui PUBLIC WITH_GUI)
endif()

# -- Define the main executable once --
add_executable(word-games src/main.cpp)

# -- Link the core game logic (profiled or not) --
target_link_libraries(word-games PRIVATE
    $<IF:$<BOOL:${WITH_TRACY}>,word-games-core-profiled,word-games-core-base>
)

# -- Conditionally link the GUI library and add its dependencies --
if(BUILD_GUI)
    target_link_libraries(word-games PRIVATE word-games-gui)
    set_target_properties(word-games PROPERTIES AUTOMOC ON)

    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${CMAKE_PREFIX_PATH}/bin)
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET word-games POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} ARGS $<TARGET_FILE:word-games>
                WORKING_DIRECTORY $<TARGET_FILE_DIR:word-games>
                COMMENT "Running windeployqt...")
        endif()
    endif()
endif()

# -- Add shared resources dependency --
add_dependencies(word-games copy_resources)

# ===================================================================
# 6. SETUP TESTING (NO GUI DEPENDENCIES)
# ===================================================================
if(BUILD_TESTING)
    message(STATUS "Testing enabled.")
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
    target_link_libraries(word-games-tests PRIVATE
        word-games-core-base
        word-games-core-objects
        gtest_main
    )
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
    message(STATUS "Testing disabled.")
endif()

# ===================================================================
# 7. SETUP BENCHMARKING (NO GUI DEPENDENCIES)
# ===================================================================
if(BUILD_BENCHMARKS)
    message(STATUS "Benchmarks enabled.")
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.4
    )
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(word-games-benchmarks benchmarks/benchmarks.cpp)
    target_link_libraries(word-games-benchmarks PRIVATE
        word-games-core-base
        word-games-core-objects
        benchmark::benchmark_main
    )
    add_dependencies(word-games-benchmarks copy_resources)
else()
    message(STATUS "Benchmarks disabled.")
endif()