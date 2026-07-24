cmake_minimum_required(VERSION 3.16)
project(QTgrid-quad LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- dependency: numeric-type-quad manages itself -----------------------------
add_subdirectory(extern/numeric-type-quad)

# ---- own paths ----------------------------------------------------------------
set(PROJECT_INCLUDE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(JSON_INCLUDE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/extern/json/include)

# ---- interface target: carries our headers + transitive deps ------------------
add_library(qtgrid_deps INTERFACE)
target_include_directories(qtgrid_deps INTERFACE
    ${PROJECT_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
)
target_link_libraries(qtgrid_deps INTERFACE numeric_type_deps)

# ---- test targets -------------------------------------------------------------
add_executable(Test_grid test/test_grid.cpp)
target_link_libraries(Test_grid PRIVATE qtgrid_deps)
target_compile_options(Test_grid PRIVATE -Wall -Wextra -Wreorder -Wshadow)