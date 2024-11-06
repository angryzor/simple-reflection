cmake_minimum_required(VERSION 3.28)
project(simple-reflection VERSION 0.0.1
                 DESCRIPTION "Simple reflection library"
                 LANGUAGES CXX)

add_library(simple-reflection INTERFACE)
target_compile_features(simple-reflection INTERFACE cxx_std_20)
add_subdirectory(src)

add_executable(simple-reflection-tests)
target_link_libraries(simple-reflection-tests PRIVATE simple-reflection)
add_subdirectory(tests)
