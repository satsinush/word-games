cmake_minimum_required(VERSION 3.16)
project(p++ LANGUAGES CXX)

# Define project version
set(p++_VERSION "3.0.0")

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
    
    # Copy ASan runtime DLL to output directory
    if(MSVC_VERSION GREATER_EQUAL 1920)
        find_file(ASAN_DLL_PATH
            NAMES clang_rt.asan_dynamic-x86_64.dll
            PATHS 
                "$ENV{VCToolsInstallDir}/bin/Hostx64/x64"
                "$ENV{VSINSTALLDIR}/VC/Tools/MSVC/*/bin/Hostx64/x64"
            NO_DEFAULT_PATH
        )
        if(ASAN_DLL_PATH)
            message(STATUS "Found ASan DLL: ${ASAN_DLL_PATH}")
        else()
            message(WARNING "ASan DLL not found. You may need to run from VS Developer Command Prompt.")
        endif()
    endif()
endif()

option(BUILD_GUI "Build the application with the Qt GUI" OFF)
option(BUILD_TESTING "Build the project's tests" OFF)
option(BUILD_BENCHMARKS "Build the project's benchmarks" OFF)
option(BUILD_TRACY "Enable Tracy profiling support" OFF)

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

add_library(p++-base STATIC ${CORE_SRCS})
target_include_directories(p++-base PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation prefix" FORCE)
endif()
set(INSTALLED_RESOURCE_DIR "${CMAKE_INSTALL_PREFIX}/resources")
set(DEVELOPMENT_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")

target_compile_definitions(p++-base PUBLIC
    "INSTALLED_RESOURCE_DIR=\"${INSTALLED_RESOURCE_DIR}\""
    "DEVELOPMENT_RESOURCE_DIR=\"${DEVELOPMENT_RESOURCE_DIR}\""
)

if(BUILD_TRACY)
    target_include_directories(p++-base PUBLIC ${tracy_SOURCE_DIR}/public)
endif()

if(MSVC)
    target_compile_options(p++-base PRIVATE /W4 /permissive-)
else()
    target_compile_options(p++-base PRIVATE -Wall -Wextra -fdiagnostics-color=always -Wno-stringop-overread)
endif()

if(BUILD_TRACY)
    add_library(p++-profiled STATIC ${CORE_SRCS})
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
    target_compile_definitions(p++-profiled PUBLIC
        "INSTALLED_RESOURCE_DIR=\"${INSTALLED_RESOURCE_DIR}\""
        "DEVELOPMENT_RESOURCE_DIR=\"${DEVELOPMENT_RESOURCE_DIR}\""
    )
    target_compile_definitions(p++-profiled PUBLIC TRACY_ENABLE=1)
    target_link_libraries(p++-profiled INTERFACE TracyClient)
endif()

# ===================================================================
# 5. CREATE THE MAIN EXECUTABLE (WITH OPTIONAL GUI)
# ===================================================================
if(BUILD_GUI)
    message(STATUS "GUI build enabled.")
    find_package(Qt6 COMPONENTS Widgets Svg REQUIRED)

    file(GLOB_RECURSE GUI_SRCS CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/gui/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/gui/*.ui"
    )
    add_library(p++-gui STATIC ${GUI_SRCS})
    target_include_directories(p++-gui PUBLIC ${CMAKE_SOURCE_DIR}/src)
    set_target_properties(p++-gui PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
    target_link_libraries(p++-gui PUBLIC Qt6::Widgets Qt6::Svg)
    target_compile_definitions(p++-gui PUBLIC WITH_GUI)
endif()

# -- Define the main executable --
if(BUILD_GUI AND WIN32)
    add_executable(p++ WIN32 src/main.cpp)
    
    # --- ROBUST ICON LOGIC (Fixes MinGW Space-in-Path Crash) ---
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icon.ico")
        set(ICON_RC_PATH "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
        file(WRITE "${ICON_RC_PATH}" "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/resources/icon.ico\"")
        
        if(MSVC)
            target_sources(p++ PRIVATE "${ICON_RC_PATH}")
        else()
            # Manual compile for MinGW to avoid windres/cc1 include path bug
            enable_language(RC)
            set(ICON_OBJ_PATH "${CMAKE_CURRENT_BINARY_DIR}/app_icon.obj")
            add_custom_command(
                OUTPUT "${ICON_OBJ_PATH}"
                COMMAND ${CMAKE_RC_COMPILER} -O coff -i "${ICON_RC_PATH}" -o "${ICON_OBJ_PATH}"
                DEPENDS "${ICON_RC_PATH}" "${CMAKE_SOURCE_DIR}/resources/icon.ico"
                VERBATIM
            )
            target_sources(p++ PRIVATE "${ICON_OBJ_PATH}")
        endif()
    endif()
else()
    add_executable(p++ src/main.cpp)
endif()

if(BUILD_TRACY)
    target_compile_definitions(p++ PRIVATE TRACY_ENABLE=1)
endif()

target_link_libraries(p++ PRIVATE
    $<IF:$<BOOL:${BUILD_TRACY}>,p++-profiled,p++-base>
)

target_compile_definitions(p++ PRIVATE
    PPLUSPLUS_VERSION="${p++_VERSION}"
)

if(BUILD_GUI)
    target_link_libraries(p++ PRIVATE p++-gui)
    set_target_properties(p++ PROPERTIES AUTOMOC ON)
    if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${CMAKE_PREFIX_PATH}/bin)
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET p++ POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} ARGS $<TARGET_FILE:p++>
                WORKING_DIRECTORY $<TARGET_FILE_DIR:p++>
                COMMENT "Running windeployqt for debug build...")
        endif()
    endif()
endif()

# Copy ASan DLL to output directory for MSVC Debug builds
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND ASAN_DLL_PATH)
    add_custom_command(TARGET p++ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASAN_DLL_PATH}" $<TARGET_FILE_DIR:p++>
        COMMENT "Copying ASan runtime DLL to output directory..."
    )
endif()

add_dependencies(p++ copy_resources)

# ===================================================================
# 6. SETUP TESTING & BENCHMARKS
# ===================================================================
if(BUILD_TESTING)
    message(STATUS "Testing enabled.")
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.17.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_executable(p++-tests tests/tests.cpp)
    target_link_libraries(p++-tests PRIVATE p++-base gtest_main)
    add_dependencies(p++-tests copy_resources)
    include(GoogleTest)
    if(WIN32 AND MINGW)
        get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
        set(TEST_ENVIRONMENT_LIST "PATH=${MINGW_BIN_DIR};$ENV{SystemRoot}/System32")
        gtest_discover_tests(p++-tests PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT_LIST}")
    else()
        gtest_discover_tests(p++-tests)
    endif()
endif()

if(BUILD_BENCHMARKS)
    message(STATUS "Benchmarks enabled.")
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googlebenchmark GIT_REPOSITORY https://github.com/google/benchmark.git GIT_TAG v1.9.4)
    FetchContent_MakeAvailable(googlebenchmark)
    add_executable(p++-benchmarks benchmarks/benchmarks.cpp)
    target_link_libraries(p++-benchmarks PRIVATE p++-base benchmark::benchmark benchmark::benchmark_main)
    add_dependencies(p++-benchmarks copy_resources)
endif()

# ===================================================================
# 8. INSTALLATION AND PACKAGING (CPack)
# ===================================================================

install(TARGETS p++ RUNTIME DESTINATION bin)
install(DIRECTORY "${RESOURCE_DIR}/" DESTINATION resources USE_SOURCE_PERMISSIONS)

if(WIN32 AND BUILD_GUI)
    qt_generate_deploy_app_script(TARGET p++ OUTPUT_SCRIPT DEPLOYMENT_SCRIPT_FILE NO_UNSUPPORTED_PLATFORM_ERROR)
    install(SCRIPT ${DEPLOYMENT_SCRIPT_FILE})
endif()

include(InstallRequiredSystemLibraries)

# --- HELPER: Sanitize Paths for NSIS ---
# This converts "C:/Path/With Spaces" -> "C:\\Path\\With Spaces"
# This allows CPack to read the path safely without confusing it for an escape code (\U)
macro(sanitize_path_for_nsis INPUT_PATH OUTPUT_VAR)
    file(TO_CMAKE_PATH "${INPUT_PATH}" _temp_path)
    string(REPLACE "/" "\\\\" _sanitized "${_temp_path}")
    set(${OUTPUT_VAR} "${_sanitized}")
endmacro()

# --- DEFINE VARIABLES BEFORE INCLUDING CPACK ---
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "Puzzle++")
set(CPACK_PACKAGE_FILE_NAME "PuzzlePlusPlus-${p++_VERSION}-win64")
set(CPACK_PACKAGE_VENDOR "Puzzle++ Team")
set(CPACK_PACKAGE_CONTACT "Puzzle++ Team")
set(CPACK_PACKAGE_VERSION "${p++_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Advanced word game and puzzle solver")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Puzzle++")

# --- 1. LICENSE FILE ---
# Sanitize the path to handle spaces in "Local Documents" (Windows only)
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    if(WIN32)
        sanitize_path_for_nsis("${CMAKE_SOURCE_DIR}/LICENSE" LICENSE_PATH)
        set(CPACK_RESOURCE_FILE_LICENSE "${LICENSE_PATH}")
    else()
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    endif()
endif()

# --- NSIS Specific Settings ---
set(CPACK_NSIS_DISPLAY_NAME "Puzzle++")
set(CPACK_NSIS_PACKAGE_NAME "Puzzle++")
set(CPACK_NSIS_MENU_LINKS "bin/p++.exe" "Puzzle++")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\p++.exe")

# --- Non-Admin (Per-User) Installation ---
# Configure NSIS to run at user level without requesting admin/UAC elevation
set(CPACK_NSIS_REQUIRED_EXECUTION_LEVEL "user")
set(CPACK_NSIS_PACKAGE_INSTALL_REGISTRY_KEY "Puzzle++")
set(CPACK_NSIS_ENABLE_HIERARCHICAL_INSTALLATION OFF)

# Set INSTDIR dynamically during pre-install execution right before directory page display
set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "
  SetShellVarContext current
  StrCpy \$INSTDIR \"\$LOCALAPPDATA\\\\Programs\\\\Puzzle++\"
")

# Explicitly instruct NSIS compiler for Current User (HKCU) installation
set(CPACK_NSIS_DEFINES "
  RequestExecutionLevel user
  !define MULTIUSER_EXECUTIONLEVEL User
  !define MULTIUSER_INSTALLMODE_DEFAULT_CURRENTUSER
")

# --- 2. ICONS & BITMAPS ---
# We sanitize these paths to ensure NSIS receives "C:\\Users\\..." 
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icon.ico")
    sanitize_path_for_nsis("${CMAKE_SOURCE_DIR}/resources/icon.ico" ICON_PATH)
    set(CPACK_NSIS_MUI_ICON "${ICON_PATH}")
    set(CPACK_NSIS_MUI_UNIICON "${ICON_PATH}")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/resources/header.bmp")
    sanitize_path_for_nsis("${CMAKE_SOURCE_DIR}/resources/header.bmp" BMP_PATH)
    set(CPACK_PACKAGE_ICON "${BMP_PATH}")
endif()

# --- INCLUDE CPACK LAST ---
include(CPack)

# ===================================================================
# 9. PACKAGING TARGET
# ===================================================================
add_custom_target(package_release_installer
    COMMAND ${CMAKE_COMMAND} --build . --target package --config Release
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building and packaging the final installer for RELEASE."
    DEPENDS p++
)