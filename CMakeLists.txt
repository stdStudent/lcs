cmake_minimum_required(VERSION 3.22)
project(lcs)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory("${PROJECT_SOURCE_DIR}/client" "${PROJECT_SOURCE_DIR}/client/bin")
add_subdirectory("${PROJECT_SOURCE_DIR}/server" "${PROJECT_SOURCE_DIR}/server/bin")
