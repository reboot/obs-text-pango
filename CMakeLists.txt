cmake_minimum_required(VERSION 2.8.12)

project(text-pango)

# Standalone build
if(NOT OBS_OUTPUT_DIR)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
    include(ObsHelpers)

    set(CMAKE_C_STANDARD 11)

    find_path(OBS_INCLUDE_DIRS
        obs-module.h)
    find_library(OBS_LIBRARIES
        NAMES libobs obs)
# Build as submodule of obs-studio
else()
    set(OBS_INCLUDE_DIRS
        ../../libobs)
    set(OBS_LIBRARIES
        libobs)
endif()

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
        pkg_check_modules(PANGO pango)
        pkg_check_modules(CAIRO cairo)
        pkg_check_modules(PANGOCAIRO pangocairo)
endif()

include_directories(
        ${OBS_INCLUDE_DIRS}
	${PANGO_INCLUDE_DIRS}
	${CAIRO_INCLUDE_DIRS}
	${PANGOCAIRO_INCLUDE_DIRS})

set(text-pango_SOURCES
	text-pango.c
	text-pango.h
	utils.c
	utils.h)

add_library(text-pango MODULE
	${text-pango_SOURCES})

target_link_libraries(text-pango
	${OBS_LIBRARIES}
	${PANGO_LIBRARIES}
	${CAIRO_LIBRARIES}
	${PANGOCAIRO_LIBRARIES})

install_obs_plugin_with_data(text-pango data)
