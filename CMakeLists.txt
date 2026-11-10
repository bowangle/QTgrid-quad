cmake_minimum_required(VERSION 3.21)
project(QTgrid-quad VERSION 0.1.0 LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Lower layers must already be installed; JSON stays in the shared extern/ tree.
find_package(numeric-type-quad CONFIG REQUIRED)
set(QTGRID_EXTERN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../extern"
    CACHE PATH "Shared external sources (JSON)")
find_path(JSON_INCLUDE_DIR nlohmann/json.hpp PATHS "${QTGRID_EXTERN_DIR}/json/include"
    NO_DEFAULT_PATH NO_CACHE REQUIRED)

option(BUILD_SHARED_LIBS "Build shared instead of static libraries" OFF)
option(QTGRID_BUILD_TESTS "Build QTgrid tests" ${PROJECT_IS_TOP_LEVEL})
add_library(qtgrid_quad
    src/grid_double.cpp src/grid_double_double.cpp src/grid_float128.cpp)
add_library(qtgrid_quad::qtgrid_quad ALIAS qtgrid_quad)
set_target_properties(qtgrid_quad PROPERTIES
    POSITION_INDEPENDENT_CODE ON INSTALL_RPATH_USE_LINK_PATH TRUE)
target_compile_features(qtgrid_quad PUBLIC cxx_std_20)
target_include_directories(qtgrid_quad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(qtgrid_quad SYSTEM PUBLIC "${JSON_INCLUDE_DIR}")
target_link_libraries(qtgrid_quad PUBLIC numeric_type_quad::numeric_type_quad)

if(QTGRID_BUILD_TESTS)
    enable_testing()
    foreach(name grid multigrid library)
        add_executable(qtgrid_test_${name} test/test_${name}.cpp)
        target_link_libraries(qtgrid_test_${name} PRIVATE qtgrid_quad)
        # Original test warnings, with assertions kept active in Release builds.
        target_compile_options(qtgrid_test_${name} PRIVATE
            "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wreorder;-Wshadow;-UNDEBUG>")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-output/${name}")
        add_test(NAME qtgrid_${name} COMMAND qtgrid_test_${name})
        set_tests_properties(qtgrid_${name} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-output/${name}")
    endforeach()
endif()

# Export the same target for independent builds of higher layers.
install(TARGETS qtgrid_quad EXPORT QTgrid-quadTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/QTgrid-quad DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
set(package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/QTgrid-quad")
install(EXPORT QTgrid-quadTargets
    NAMESPACE qtgrid_quad:: DESTINATION "${package_dir}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/QTgrid-quadConfig.cmake" [=[
include(CMakeFindDependencyMacro)
find_dependency(numeric-type-quad CONFIG)
include("${CMAKE_CURRENT_LIST_DIR}/QTgrid-quadTargets.cmake")
]=])
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QTgrid-quadConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QTgrid-quadConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QTgrid-quadConfigVersion.cmake"
    DESTINATION "${package_dir}")
