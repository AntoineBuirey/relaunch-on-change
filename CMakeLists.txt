cmake_minimum_required(VERSION 3.16)

project(relaunch-on-change
    VERSION 1.0
    LANGUAGES CXX
)

add_executable(relaunch-on-change
    src/main.cpp
    src/process.cpp
    src/scanner.cpp
)

target_include_directories(relaunch-on-change
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/header
)

target_compile_features(relaunch-on-change PRIVATE cxx_std_17)