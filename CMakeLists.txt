cmake_minimum_required(VERSION 3.16)
project(word-games LANGUAGES CXX)

# ===============================================================
# 1. PROJECT SETTINGS & USER OPTIONS
# ===============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_GUI "Build with the Qt GUI" ON)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(BUILD_TRACY "Enable Tracy profiling" ON)

# ===============================================================
# 2. STATIC LINKING ON WINDOWS (MinGW)
# ===============================================================
if(WIN32 AND MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    message(STATUS "Statically linking C++ runtime (MinGW).")
endif()

# ===============================================================
# 3. BASE LIBRARY (NO GUI / NO TRACY)
# ===============================================================
file(GLOB_RECURSE BASE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
# Exclude GUI and main
list(FILTER BASE_SRCS EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/src/gui/.*")
list(FILTER BASE_SRCS EXCLUDE REGEX ".*/main.cpp")

add_library(word-games-base STATIC ${BASE_SRCS})
target_include_directories(word-games-base PUBLIC ${CMAKE_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(word-games-base PRIVATE /W4 /permissive-)
else()
    target_compile_options(word-games-base PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()

# ===============================================================
# 4. CORE LIBRARY (Adds Tracy / profiling layer)
# ===============================================================
add_library(word-games-profiled INTERFACE)
target_link_libraries(word-games-profiled INTERFACE word-games-base)

if(BUILD_TRACY)
    message(STATUS "Tracy profiling enabled.")
    include(FetchContent)
    FetchContent_Declare(tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.12.2
    )
    FetchContent_MakeAvailable(tracy)

    target_include_directories(word-games-profiled INTERFACE ${tracy_SOURCE_DIR}/public)
    target_compile_definitions(word-games-profiled INTERFACE TRACY_ENABLE)
    target_link_libraries(word-games-profiled INTERFACE TracyClient)
endif()

# ===============================================================
# 5. GUI LIBRARY (Optional)
# ===============================================================
if(BUILD_GUI)
    message(STATUS "Building Qt GUI.")
    find_package(Qt6 COMPONENTS Widgets REQUIRED)

    file(GLOB_RECURSE GUI_SRCS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/gui/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.ui"
    )

    add_library(word-games-gui STATIC ${GUI_SRCS})
    target_include_directories(word-games-gui PUBLIC ${CMAKE_SOURCE_DIR}/src)
    set_target_properties(word-games-gui PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

    target_link_libraries(word-games-gui PUBLIC Qt6::Widgets word-games-base)
    target_compile_definitions(word-games-gui PUBLIC WITH_GUI)

    if(BUILD_TRACY)
        target_compile_definitions(word-games-gui PUBLIC TRACY_ENABLE)
        target_link_libraries(word-games-gui PUBLIC TracyClient)
    endif()
endif()

# ===============================================================
# 6. MAIN EXECUTABLE (Optionally GUI + Tracy)
# ===============================================================
add_executable(word-games src/main.cpp)
target_link_libraries(word-games PRIVATE word-games-profiled)

if(BUILD_GUI)
    target_link_libraries(word-games PRIVATE word-games-gui)
    set_target_properties(word-games PROPERTIES AUTOMOC ON)
endif()

if(BUILD_TRACY)
    target_compile_definitions(word-games PRIVATE TRACY_ENABLE)
endif()

# Propagate defines for IntelliSense (VSCode)
if(BUILD_GUI)
    target_compile_definitions(word-games PRIVATE WITH_GUI)
endif()
if(BUILD_TRACY)
    target_compile_definitions(word-games PRIVATE TRACY_ENABLE)
endif()

# ---------------------------------------------------------------
# Windows: Auto-run windeployqt to bundle correct Qt DLLs
# ---------------------------------------------------------------
if(WIN32 AND BUILD_GUI)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_HOST_PATH}/bin" "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET word-games POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:word-games>"
            COMMENT "Running windeployqt to copy Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found — Qt DLLs will not be bundled automatically.")
    endif()
endif()

# ===============================================================
# 7. RESOURCES
# ===============================================================
set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${RESOURCE_DIR} ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying shared resources..."
)
add_dependencies(word-games copy_resources)

# ===============================================================
# 8. TESTS (No GUI / No Tracy)
# ===============================================================
if(BUILD_TESTING)
    message(STATUS "Building tests.")
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(word-games-tests tests/tests.cpp)
    target_link_libraries(word-games-tests PRIVATE
        word-games-base
        gtest_main
    )
    add_dependencies(word-games-tests copy_resources)
    target_compile_definitions(word-games-tests PRIVATE NO_GUI NO_TRACY)

    include(GoogleTest)
    gtest_discover_tests(word-games-tests)
endif()

# ===============================================================
# 9. BENCHMARKS (No GUI / No Tracy)
# ===============================================================
if(BUILD_BENCHMARKS)
    message(STATUS "Building benchmarks.")
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.4
    )
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(word-games-benchmarks benchmarks/benchmarks.cpp)
    target_link_libraries(word-games-benchmarks PRIVATE
        word-games-base
        benchmark::benchmark_main
    )
    add_dependencies(word-games-benchmarks copy_resources)
    target_compile_definitions(word-games-benchmarks PRIVATE NO_GUI NO_TRACY)
endif()