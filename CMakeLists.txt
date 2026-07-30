cmake_minimum_required(VERSION 3.21)
project(QTgrid-quad LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(QTGRID_BUILD_TESTS
       "Build QTgrid-quad's own tests"
       ${PROJECT_IS_TOP_LEVEL})

# ---- dependency: numeric-type-quad manages itself -----------------------------
# Its tests stay off unless NUMERIC_TYPE_BUILD_TESTS is explicitly set,
# because it is no longer the top-level project.
add_subdirectory(extern/numeric-type-quad)

# ---- own paths ----------------------------------------------------------------
set(QTGRID_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(JSON_INCLUDE_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/extern/json/include)

if(NOT EXISTS ${JSON_INCLUDE_DIR}/nlohmann/json.hpp)
    message(FATAL_ERROR
        "nlohmann/json not found at ${JSON_INCLUDE_DIR}.\n"
        "Run: git submodule update --init --recursive")
endif()

# ---- interface target: carries our headers + transitive deps ------------------
add_library(qtgrid_deps INTERFACE)

target_include_directories(qtgrid_deps INTERFACE
    ${QTGRID_INCLUDE_DIR})

target_include_directories(qtgrid_deps SYSTEM INTERFACE
    ${JSON_INCLUDE_DIR})

target_compile_features(qtgrid_deps INTERFACE cxx_std_20)

target_link_libraries(qtgrid_deps INTERFACE numeric_type_deps)

# ---- tests (leaf targets: nothing may ever depend on these) -------------------
if(QTGRID_BUILD_TESTS)
    enable_testing()

    foreach(_name grid multigrid)
        add_executable(qtgrid_test_${_name} test/test_${_name}.cpp)
        target_link_libraries(qtgrid_test_${_name} PRIVATE qtgrid_deps)
        target_compile_options(qtgrid_test_${_name} PRIVATE
            -Wall -Wextra -Wreorder -Wshadow)
        add_test(NAME qtgrid_test_${_name} COMMAND qtgrid_test_${_name})
    endforeach()
endif()