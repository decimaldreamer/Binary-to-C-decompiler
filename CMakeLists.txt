cmake_minimum_required(VERSION 3.10)
project(binary_to_c_decompiler VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Find required packages
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(OpenMP)

# Main executable
add_executable(b2c_decompiler
    main.c
    opsoup.h
    ref.c
    disasm.c
    elf.c
    pe.c
    macho.c
    image.c
    label.c
    data.c
    optimize.c
    analysis.c
)

# GUI executable
add_executable(b2c_gui
    gui/mainwindow.cpp
    gui/mainwindow.h
)

# Include directories
target_include_directories(b2c_decompiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(b2c_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
target_link_libraries(b2c_decompiler PRIVATE OpenMP::OpenMP_C)
target_link_libraries(b2c_gui PRIVATE Qt6::Widgets)

# Install targets
install(TARGETS b2c_decompiler b2c_gui DESTINATION bin)

# Add subdirectories
add_subdirectory(tests) 