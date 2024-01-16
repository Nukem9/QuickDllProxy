cmake_minimum_required(VERSION 3.26)

project(
    dllproxy-examples
    VERSION 1.0
    LANGUAGES CXX
)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/demo_dbghelp")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/demo_winhttp")