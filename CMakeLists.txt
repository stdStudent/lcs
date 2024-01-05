cmake_minimum_required(VERSION 3.22)
project(lcs)

set(CMAKE_CXX_STANDARD 20)
list(APPEND CMAKE_PREFIX_PATH "/usr/include")
set(CMAKE_FIND_ROOT_PATH "/usr/include")

# -------
# Helpers
file(GLOB_RECURSE HELPER_FILES ${PROJECT_SOURCE_DIR}/helpers/*.h ${PROJECT_SOURCE_DIR}/helpers/*.cpp)
add_library(helpers ${HELPER_FILES})
target_include_directories(helpers PUBLIC ${PROJECT_SOURCE_DIR}/helpers)
set_target_properties(helpers PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(helpers config++)

# -----------------------------------
# Actual client and server submodules
add_subdirectory("${PROJECT_SOURCE_DIR}/client" "${PROJECT_SOURCE_DIR}/client/bin")
target_link_libraries(client helpers)

add_subdirectory("${PROJECT_SOURCE_DIR}/server" "${PROJECT_SOURCE_DIR}/server/bin")
target_link_libraries(server helpers)
