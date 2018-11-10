cmake_minimum_required (VERSION 3.1.3)
include(GNUInstallDirs)

## Project
project (AudioMixer C)

## Build
set (TARGET demo)
add_executable (${TARGET} "")

## Sources
target_sources(
    ${TARGET}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/main.c
)

## Headers
find_path(INC_GLIB glib/galloca.h PATH_SUFFIXES glib-2.0)
include_directories(${INC_GLIB})

find_path(INC_GLIBCONF glibconfig.h HINTS /usr/lib/x86_64-linux-gnu PATH_SUFFIXES glib-2.0/include)
include_directories(${INC_GLIBCONF})

target_include_directories (
    ${TARGET}
    PRIVATE
        /usr/include/gstreamer-1.0
        /usr/lib/x86_64-linux-gnu/gstreamer-1.0/include/
)

# Link external/3rd party libs
target_link_libraries (
    ${TARGET}
    PRIVATE
        glib-2.0
        gobject-2.0
        gstreamer-1.0
        pthread
)

# Compilation Params
target_compile_options (
    ${TARGET}
    PRIVATE
        -std=c99
        -Wall
        -Werror
        -Wunused
        -pedantic
)

# Custom Commands

# Installation
install (
    TARGETS         ${TARGET}
    DESTINATION     ${PROJECT_SOURCE_DIR}/build
)