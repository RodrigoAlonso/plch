cmake_minimum_required(VERSION 3.10)

project(PlcHitachiWinNative
    VERSION 0.1
    DESCRIPTION "PLC Hitachi Native Windows Application")

option(PLCH_BUILD_GUI "Build the SDL_bgi simulator (plc)" ON)

# Simulation engine as a standalone library (see libplcsim/)
enable_testing()
add_subdirectory(libplcsim)

if(PLCH_BUILD_GUI)
# 1. Locate the native MSYS2 SDL2 configurations
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# 2. Gather all project source files from the src directory
set(PROJECT_SOURCES
    src/plcmain.cpp
    src/plc.cpp
    src/interfaz.cpp
    src/mostprog.cpp
    src/soporte.cpp
    src/ventanas.cpp
)

# 3. Add the executable targeting native Windows sub-system (-mwindows)
add_compile_options(-Wno-write-strings)
add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})

# Change the actual compiled file name to 'custom_output_name'
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "plc")

# 4. Handle configuration file header templates
#configure_file(config.h.in config.h)

# 5. Set include directories for compiler search paths
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../VENTANAS" # Allows #include "ventanas.h" to work
    ${SDL2_INCLUDE_DIRS}
)

# 6. Link libraries in the strict required hierarchy order
target_link_libraries(${PROJECT_NAME} PRIVATE
    plcsim::plcsim
    SDL2_bgi
    ${SDL2_LIBRARIES}
)

# 7. Set compilation engine properties to modern standard rules
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif()
