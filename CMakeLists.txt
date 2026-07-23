cmake_minimum_required(VERSION 3.26..4.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  supernova
  VERSION 1.0
  LANGUAGES CXX)

set(SRC
    src/main.cpp
    src/multifile_shaders.cpp
    src/explosions.cpp
    src/audio_engine.cpp
    src/audio/miniaudio.cpp
    src/snmesh.cpp
    src/oldman.cpp
    src/video_export.cpp)

# install framework
include(FetchContent)
FetchContent_Declare(
  framework URL https://github.com/julcst/gltemplate/archive/v1.7b.tar.gz)
FetchContent_MakeAvailable(framework)

# Find FFmpeg components.
#
# Two ways to get there, both ending at the supernova_ffmpeg target: -
# pkg-config, which is how devenv/nix provides ffmpeg on Linux -
# find_package(FFMPEG), the module vcpkg's ffmpeg port ships on Windows; it sets
# FFMPEG_* variables and no imported target pkg-config is tried first and only
# as a preference, so a machine without it is not an error - it just falls
# through to the second path.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(
    FFMPEG
    QUIET
    IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswresample
    libswscale)
endif()

add_library(supernova_ffmpeg INTERFACE)
if(TARGET PkgConfig::FFMPEG)
  target_link_libraries(supernova_ffmpeg INTERFACE PkgConfig::FFMPEG)
else()
  find_package(FFMPEG REQUIRED)
  target_include_directories(supernova_ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
  target_link_directories(supernova_ffmpeg INTERFACE ${FFMPEG_LIBRARY_DIRS})
  target_link_libraries(supernova_ffmpeg INTERFACE ${FFMPEG_LIBRARIES})
endif()

# compile project
add_executable(${PROJECT_NAME} ${SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE framework supernova_ffmpeg)
