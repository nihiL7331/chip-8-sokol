# I don't know how to write CMake files.
# Hence everything is commented here as a file
# I can come back to if I forget something in a
# different project.
# <3

# 'cmake -B build' tells CMake to read the file and generate
# the build scripts inside a new folder called 'build'.
#
# 'cmake --build build' tells CMake to actually execute the
# generated scripts and compile C++ code to a binary.

# Always start by defining the minimum version required
cmake_minimum_required(VERSION 3.20)

# Name the project and set the language to C++
project(Chip8Sokol LANGUAGES CXX)

# This line ensures CMake generates compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force the compiler to use modern C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake what kind of executable we want to build.
# Need to list every .cpp file that needs to be compiled here.
add_executable(chip8
  src/main.cpp
  src/sokol.cpp
  src/chip8.cpp
)

# Tell the target where to find external header files.
# PRIVATE means these headers are only used to build this specific executable.
target_include_directories(chip8 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

# Sokol requires the OS's native grahpics/audio libs to open a window.
if (APPLE)
  # On macOS treat C++ as Objective-C++
  enable_language(OBJCXX)
  set_source_files_properties(src/sokol.cpp PROPERTIES LANGUAGE OBJCXX)
    
  target_link_libraries(chip8 PRIVATE
    "-framework Cocoa"
    "-framework QuartzCore"
    "-framework OpenGL"
    "-framework AudioToolbox"
  )
elseif (UNIX AND NOT APPLE) # Linux
  target_link_libraries(chip8 PRIVATE
    GL
    dl
    m
    pthread
    X11
    Xi
    Xcursor
    Xrandr
    asound  
  )
elseif (WIN32)
  target_link_libraries(chip8 PRIVATE
    kernel32
    user32
    shell32
    gdi32
    d3d11
    dxgi
  )
endif()
