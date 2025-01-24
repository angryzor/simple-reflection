cmake_minimum_required(VERSION 3.28)
project(simple-reflection VERSION 1.0.2
                 DESCRIPTION "Simple reflection library"
                 LANGUAGES CXX)

add_library(simple-reflection INTERFACE)
target_compile_features(simple-reflection INTERFACE cxx_std_20)
add_subdirectory(src)

add_executable(simple-reflection-tests)
target_link_libraries(simple-reflection-tests PRIVATE simple-reflection)
add_subdirectory(tests)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(simple-reflection-config-version.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
configure_file(cmake/simple-reflection-config.cmake "${CMAKE_CURRENT_BINARY_DIR}/simple-reflection-config.cmake" COPYONLY)
export(TARGETS simple-reflection FILE simple-reflection-targets.cmake)
export(PACKAGE simple-reflection)
