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
    src/meshes.cpp
    src/oldman.cpp)

# install framework
include(FetchContent)
FetchContent_Declare(
  framework URL https://github.com/julcst/gltemplate/archive/v1.7b.tar.gz)
FetchContent_MakeAvailable(framework)

# compile project
add_executable(${PROJECT_NAME} ${SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE framework)
