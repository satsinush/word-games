cmake_minimum_required(VERSION 3.16)
project(p++ LANGUAGES CXX)

# ===================================================================
# 1. PROJECT-WIDE SETTINGS
# ===================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable AddressSanitizer (ASan) for MSVC Debug builds
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "MSVC AddressSanitizer (ASan) enabled.")
    add_compile_options(/fsanitize=address)
endif()

option(BUILD_GUI "Build the application with the Qt GUI" ON)
option(BUILD_TESTING "Build the project's tests" ON)
option(BUILD_BENCHMARKS "Build the project's benchmarks" ON)
option(BUILD_TRACY "Enable Tracy profiling support" ON)

# Statically link C++ standard library and runtime on Windows with MinGW
# if(WIN32 AND MINGW)
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
#     set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
#     message(STATUS "Statically linking C++ standard library and runtime")
# endif()

# ===================================================================
# 2. GATHER CORE SOURCE FILES
# ===================================================================
file(GLOB_RECURSE CORE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER CORE_SRCS EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/src/gui/.*")
list(FILTER CORE_SRCS EXCLUDE REGEX ".*/main.cpp")

# ===================================================================
# 3. SETUP SHARED RESOURCES
# ===================================================================
set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)

set(COPY_DESTINATION "${CMAKE_BINARY_DIR}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,$<CONFIG>/,./>resources")

add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${RESOURCE_DIR}
            "${COPY_DESTINATION}"
    COMMENT "Copying shared resources..."
)

# ===================================================================
# 4. SETUP TRACY AND CREATE LIBRARY VARIANTS
# ===================================================================
# --- Fetch Tracy first (always, even if not enabled) ---
if(BUILD_TRACY)
    message(STATUS "Tracy profiling enabled.")
    include(FetchContent)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG        v0.12.2
    )
    FetchContent_MakeAvailable(tracy)
endif()

# --- Create the standard, non-profiled library ---
add_library(p++-base STATIC ${CORE_SRCS})
target_include_directories(p++-base PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add Tracy include path to base library (for IntelliSense), but don't enable it
# DO NOT define TRACY_ENABLE here - leave it undefined so Tracy macros become no-ops
if(BUILD_TRACY)
    target_include_directories(p++-base PUBLIC
        ${tracy_SOURCE_DIR}/public
    )
endif()

if(MSVC)
    target_compile_options(p++-base PRIVATE /W4 /permissive-)
else()
    target_compile_options(p++-base PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()

# --- Conditionally create the profiled library variant ---
if(BUILD_TRACY)
    # Create the profiled library from the same sources
    add_library(p++-profiled STATIC ${CORE_SRCS})

    # Apply all the same base properties
    target_include_directories(p++-profiled PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
        ${tracy_SOURCE_DIR}/public
    )
    if(MSVC)
        target_compile_options(p++-profiled PRIVATE /W4 /permissive-)
    else()
        target_compile_options(p++-profiled PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
    endif()

    # Apply the Tracy-specific properties to this version
    target_compile_definitions(p++-profiled PUBLIC TRACY_ENABLE=1)
    target_link_libraries(p++-profiled INTERFACE TracyClient)
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
    add_library(p++-gui STATIC ${GUI_SRCS})

    target_include_directories(p++-gui PUBLIC ${CMAKE_SOURCE_DIR}/src)
    set_target_properties(p++-gui PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

    # Link the GUI library to its dependencies
    target_link_libraries(p++-gui PUBLIC Qt6::Widgets)
    target_compile_definitions(p++-gui PUBLIC WITH_GUI)
endif()

# -- Define the main executable once --
add_executable(p++ src/main.cpp)

if(BUILD_TRACY)
    target_compile_definitions(p++ PRIVATE TRACY_ENABLE=1)
endif()

# -- Link the core game logic (profiled or not) --
target_link_libraries(p++ PRIVATE
    $<IF:$<BOOL:${BUILD_TRACY}>,p++-profiled,p++-base>
)

# -- Conditionally link the GUI library and add its dependencies --
if(BUILD_GUI)
    target_link_libraries(p++ PRIVATE p++-gui)
    set_target_properties(p++ PROPERTIES AUTOMOC ON)

    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${CMAKE_PREFIX_PATH}/bin)
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET p++ POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} ARGS $<TARGET_FILE:p++>
                WORKING_DIRECTORY $<TARGET_FILE_DIR:p++>
                COMMENT "Running windeployqt...")
        endif()
    endif()
endif()

# -- Add shared resources dependency --
add_dependencies(p++ copy_resources)

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

    add_executable(p++-tests tests/tests.cpp)
    target_link_libraries(p++-tests PRIVATE
        p++-base
        gtest_main
    )
    add_dependencies(p++-tests copy_resources)

    include(GoogleTest)
    if(WIN32 AND MINGW)
        get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
        set(SYSTEM32_PATH "$ENV{SystemRoot}/System32")
        set(TEST_ENVIRONMENT_LIST "PATH=${MINGW_BIN_DIR};${SYSTEM32_PATH}")
        gtest_discover_tests(p++-tests
            PROPERTIES
                ENVIRONMENT "${TEST_ENVIRONMENT_LIST}"
        )
    else()
        gtest_discover_tests(p++-tests)
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

    # set(BENCHMARK_ENABLE_STATIC_LIB ON CACHE BOOL "Force static build for benchmark" FORCE)
    # set(BENCHMARK_ENABLE_SHARED_LIB OFF CACHE BOOL "Force static build for benchmark" FORCE)

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.4
    )
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(p++-benchmarks benchmarks/benchmarks.cpp)
    target_link_libraries(p++-benchmarks PRIVATE
        p++-base
        benchmark::benchmark
        benchmark::benchmark_main
    )
    add_dependencies(p++-benchmarks copy_resources)
else()
    message(STATUS "Benchmarks disabled.")
endif()