cmake_minimum_required(VERSION 3.4.1)
include_directories(src/main/cpp/fluidsynth)
add_library(scherzo-lib SHARED
             src/main/cpp/scherzo.c
             src/main/cpp/scherzo-lib.c)

find_library(log-lib log)
find_library(opensles-lib OpenSLES)
target_link_libraries(scherzo-lib ${log-lib} ${opensles-lib})
