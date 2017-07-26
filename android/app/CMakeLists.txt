cmake_minimum_required(VERSION 3.2)

add_subdirectory(../../core ${CMAKE_CURRENT_BINARY_DIR}/libscherzo_build)

add_library(scherzo-jni SHARED src/main/cpp/scherzo-lib.c)
target_include_directories(scherzo-jni PRIVATE vendor ${scherzo-core_SOURCE_DIR})
target_link_libraries(scherzo-jni libscherzo log OpenSLES android)
