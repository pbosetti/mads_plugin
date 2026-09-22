#   ____  _             _       
#  |  _ \| |_   _  __ _(_)_ __  
#  | |_) | | | | |/ _` | | '_ \ 
#  |  __/| | |_| | (_| | | | | |
#  |_|   |_|\__,_|\__, |_|_| |_|
#                 |___/         
cmake_minimum_required(VERSION 3.20)
project(mads_plugin VERSION 0.1.0 LANGUAGES CXX)

set(PLUGIN_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(PLUGIN_IS_TOP_LEVEL ON)
endif()

if(PLUGIN_IS_TOP_LEVEL AND CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

option(PLUGIN_ENABLE_EXAMPLES "Build example plugins and loaders" OFF)
if(PLUGIN_ENABLE_EXAMPLES)
  option(MADS_NO_DEPS_ONLY "Build only examples without external dependencies" OFF)
endif()

# Off by default: a plain FetchContent consumer only gets the headers in
# src/, exposed through the MADS::Plugin interface target below. Turning
# this on additionally compiles the reusable classes that live directly
# under examples/ (currently just SerialPort) into a separate static
# library, MADS::PluginAddonsLibrary, which a dependent project links
# explicitly to get both the compiled code and examples/ on its header
# search path, so it can `#include <serialport.hpp>` (and whatever else
# lands there in the future) without vendoring the .cpp file itself. It
# is not linked into MADS::Plugin, so a plain plugin consumer does not
# pay for it. This option is independent of PLUGIN_ENABLE_EXAMPLES, which
# builds the full demo plugin/loader/test tree and is meant for
# developing this repo, not for consuming it.
option(MADS_PLUGIN_BUILD_ADDONS
  "Compile examples/*.cpp into the MADS::PluginAddonsLibrary static library"
  OFF
)

# PROJECT SETTINGS #############################################################
if(PLUGIN_IS_TOP_LEVEL)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
set(USR_DIR ${CMAKE_CURRENT_LIST_DIR}/usr)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(EXAMPLES_DIR ${CMAKE_CURRENT_LIST_DIR}/examples)
if(PLUGIN_IS_TOP_LEVEL)
  set(FETCHCONTENT_QUIET OFF)
endif()

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

# DEPENDENCIES #################################################################
include(FetchContent)

# pugg is for the plugin system.
if(NOT TARGET pugg)
  FetchContent_Declare(pugg
    GIT_REPOSITORY https://github.com/pbosetti/pugg.git
    GIT_TAG        1.2.0
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(pugg)
endif()

# Plugins deal with I/O as JSON objects.
if(NOT TARGET nlohmann_json::nlohmann_json)
  set(JSON_BuildTests OFF CACHE INTERNAL "")
  FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# PUBLIC TARGET ################################################################
if(NOT TARGET mads_plugin)
  add_library(mads_plugin INTERFACE)
  target_compile_features(mads_plugin INTERFACE cxx_std_17)
  target_include_directories(mads_plugin
    INTERFACE
      $<BUILD_INTERFACE:${SRC_DIR}>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(mads_plugin
    INTERFACE
      pugg
      nlohmann_json::nlohmann_json
  )
endif()
if(NOT TARGET MADS::Plugin)
  add_library(MADS::Plugin ALIAS mads_plugin)
endif()
if(NOT TARGET mads_plugin::mads_plugin)
  add_library(mads_plugin::mads_plugin ALIAS mads_plugin)
endif()

# ADDONS (OPT-IN) ###############################################################
# Only the sources directly under examples/ are picked up: examples/plugin,
# examples/main and examples/test hold demo executables (several with their
# own main()), not code meant to be linked into a dependent project.
if(MADS_PLUGIN_BUILD_ADDONS)
  file(GLOB MADS_PLUGIN_ADDONS_SOURCES CONFIGURE_DEPENDS ${EXAMPLES_DIR}/*.cpp)
  if(NOT TARGET mads_plugin_addons)
    add_library(mads_plugin_addons STATIC ${MADS_PLUGIN_ADDONS_SOURCES})
    # SerialPort uses std::span, designated initializers and ranges.
    target_compile_features(mads_plugin_addons PUBLIC cxx_std_20)
    target_include_directories(mads_plugin_addons
      PUBLIC
        $<BUILD_INTERFACE:${EXAMPLES_DIR}>
        $<INSTALL_INTERFACE:include>
    )
  endif()
  if(NOT TARGET MADS::PluginAddonsLibrary)
    add_library(MADS::PluginAddonsLibrary ALIAS mads_plugin_addons)
  endif()
  # Deliberately not linked into mads_plugin: a plain MADS::Plugin consumer
  # must not pay for the addons. A project that wants SerialPort links
  # MADS::PluginAddonsLibrary explicitly (in addition to MADS::Plugin, or
  # alone if it has no other use for the plugin system).
endif()

# EXAMPLES #####################################################################
if(PLUGIN_ENABLE_EXAMPLES)
  enable_testing()
  add_subdirectory(examples)
endif()

# INSTALL ######################################################################
if(PLUGIN_IS_TOP_LEVEL)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${USR_DIR})
  endif()

  install(DIRECTORY ${SRC_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
  )
  if(MADS_PLUGIN_BUILD_ADDONS)
    install(DIRECTORY ${EXAMPLES_DIR}/
      DESTINATION include
      FILES_MATCHING PATTERN "*.hpp"
    )
  endif()
endif()

# DOCUMENTATION ################################################################
if(PLUGIN_IS_TOP_LEVEL)
  include(FindDoxygen)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/doc)
      file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doc)
    endif()
    message(STATUS "Doxygen found, documentation will be built")
    file(GLOB MARKDOWN_FILES "${CMAKE_CURRENT_LIST_DIR}/*.md")
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doc)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_LIST_DIR}/README.md)
    set(DOXYGEN_EXCLUDE_PATTERNS
      clock.cpp
      echo*.cpp
      twice.cpp
      webcam.cpp
      */examples/main/*
      */examples/test/*
    )
    doxygen_add_docs(Doxygen
      ${SRC_DIR} ${CMAKE_CURRENT_LIST_DIR}/examples ${MARKDOWN_FILES}
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doc
      CONFIG_FILE ${CMAKE_CURRENT_LIST_DIR}/doc/Doxyfile
    )
  endif()
endif()
