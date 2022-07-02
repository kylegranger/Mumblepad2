cmake_minimum_required(VERSION 3.1)

project(mumblepad)

if (USE_OPENGL)
    add_definitions(-DUSE_OPENGL)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")



add_subdirectory(lib)
add_subdirectory(test)