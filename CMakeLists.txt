cmake_minimum_required(VERSION 3.16)
project(word-games LANGUAGES CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- The Key Change: Add a build option ----
option(BUILD_WITH_GUI "Build the application with the Qt GUI" ON)


# Glob for core/CLI source files, excluding the gui directory
file(GLOB_RECURSE CORE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(FILTER CORE_SRCS EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/src/gui/.*")

# --- Change the condition to check our new option ---
if(BUILD_WITH_GUI)
    message(STATUS "GUI build enabled. Searching for Qt6...")
    # Find Qt6 and make it required for a GUI build
    find_package(Qt6 COMPONENTS Widgets REQUIRED)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    # Glob for all GUI-related files
    file(GLOB_RECURSE GUI_SRCS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/gui/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.ui"
    )
    # Add the GUI files to the main source list
    list(APPEND CORE_SRCS ${GUI_SRCS})
else()
    message(STATUS "GUI build disabled. Building CLI-only version.")
endif()


# Create the executable with the final list of all necessary source files
add_executable(word-games ${CORE_SRCS})

# Apply settings that are common to both build types
target_include_directories(word-games PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Apply settings only if we are building the GUI version
if(BUILD_WITH_GUI)
    target_compile_definitions(word-games PRIVATE WITH_GUI)
    target_link_libraries(word-games PRIVATE Qt6::Widgets)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(word-games PRIVATE /W4 /permissive-)
else()
    target_compile_options(word-games PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()

# ===================================================================
# AUTOMATIC DLL DEPLOYMENT
# ===================================================================
if(WIN32)
    # 1. Find the windeployqt executable
    find_program(
        WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS ${CMAKE_PREFIX_PATH}/bin
    )

    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "windeployqt found: ${WINDEPLOYQT_EXECUTABLE}")

        # 2. Add a command that runs after the build is finished
        add_custom_command(
            TARGET word-games POST_BUILD
            
            # The actual command to run
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
            
            # Arguments for the command (no --debug flag needed)
            ARGS
                $<TARGET_FILE:word-games>

            # Tell the command where to run
            WORKING_DIRECTORY $<TARGET_FILE_DIR:word-games>
            COMMENT "Running windeployqt to deploy dependencies..."
        )
    else()
        message(WARNING "windeployqt not found. DLLs will not be deployed automatically.")
    endif()
endif()