cmake_minimum_required(VERSION 3.14)

project(plcsim
    VERSION 0.1.0
    DESCRIPTION "Hitachi EC-series PLC simulation engine"
    LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Shared by default when built on its own; follows the parent otherwise.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(PLCSIM_TOP_LEVEL ON)
else()
    set(PLCSIM_TOP_LEVEL OFF)
endif()
if(PLCSIM_TOP_LEVEL AND NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()
option(PLCSIM_BUILD_TESTS "Build the plcsim tests" ${PLCSIM_TOP_LEVEL})
option(PLCSIM_BUILD_EXAMPLES "Build the plcsim examples" ${PLCSIM_TOP_LEVEL})

add_library(plcsim
    src/api.cpp
    src/dump.cpp
    src/engine.cpp
    src/loader.cpp
    src/clock.cpp
)
add_library(plcsim::plcsim ALIAS plcsim)

target_include_directories(plcsim PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(plcsim PRIVATE cxx_std_17)
target_compile_definitions(plcsim PRIVATE PLCSIM_BUILDING)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(plcsim PUBLIC PLCSIM_STATIC)
endif()

# Only the plcsim_* API is exported, and the C++ runtime is not needed, so
# the library can be linked from C without libstdc++.
set_target_properties(plcsim PROPERTIES
    PUBLIC_HEADER include/plcsim.h
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(plcsim PRIVATE
        -Wall -Wextra -fno-exceptions -fno-rtti)
endif()

#-----------------------------------------------------------------------------
# Install: headers, library, CMake package (find_package(plcsim)) and
# pkg-config file (pkg-config --cflags --libs plcsim).
install(TARGETS plcsim EXPORT plcsimTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT plcsimTargets
    NAMESPACE plcsim::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plcsim
)
configure_package_config_file(cmake/plcsimConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/plcsimConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plcsim
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/plcsimConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/plcsimConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/plcsimConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plcsim
)

if(BUILD_SHARED_LIBS)
    set(PLCSIM_PC_CFLAGS "")
else()
    set(PLCSIM_PC_CFLAGS "-DPLCSIM_STATIC")
endif()
configure_file(plcsim.pc.in ${CMAKE_CURRENT_BINARY_DIR}/plcsim.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plcsim.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

#-----------------------------------------------------------------------------
if(PLCSIM_BUILD_EXAMPLES)
    add_executable(plcsim_kitt examples/kitt.c)
    target_link_libraries(plcsim_kitt PRIVATE plcsim::plcsim)
    add_executable(plcsim_dump examples/dump.c)
    target_link_libraries(plcsim_dump PRIVATE plcsim::plcsim)
endif()

if(PLCSIM_BUILD_TESTS)
    enable_testing()
    add_executable(plcsim_test tests/test_plcsim.c)
    target_link_libraries(plcsim_test PRIVATE plcsim::plcsim)
    # Sample programs of the simulator, when built inside its repository.
    set(PLCSIM_TEST_DATA ${CMAKE_CURRENT_SOURCE_DIR}/../resources)
    if(EXISTS ${PLCSIM_TEST_DATA}/kitt.epg)
        add_test(NAME plcsim_test COMMAND plcsim_test ${PLCSIM_TEST_DATA})
    else()
        add_test(NAME plcsim_test COMMAND plcsim_test)
    endif()
endif()
