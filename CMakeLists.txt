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

# Find FFmpeg components
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVFILTER REQUIRED IMPORTED_TARGET libavfilter)
pkg_check_modules(AVDEVICE REQUIRED IMPORTED_TARGET libavdevice)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
# pkg_check_modules( FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat
# libavutil libswscale)

# compile project
add_executable(${PROJECT_NAME} ${SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE framework PkgConfig::AVCODEC PkgConfig::AVFORMAT
                          PkgConfig::AVUTIL PkgConfig::SWSCALE)
