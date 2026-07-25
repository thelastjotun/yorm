cmake_minimum_required(VERSION 3.18)
project(yorm)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_VALGRIND_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_TOOLS OFF CACHE BOOL "" FORCE)
set(ENABLE_COMMON_TARGETS OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory(submodules/libyang)
add_subdirectory(submodules/fmt)

add_executable(yorm-gen 
    src/main.cpp 
    src/yang_parser.cpp
)

target_include_directories(yorm-gen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/submodules/libyang/src
    ${CMAKE_CURRENT_BINARY_DIR}/submodules/libyang/libyang
)

target_link_libraries(yorm-gen PRIVATE yang fmt::fmt)

# Tests
option(YORM_BUILD_TESTS "Build yorm test executables" OFF)
if(YORM_BUILD_TESTS)
    add_subdirectory(test)
endif()
