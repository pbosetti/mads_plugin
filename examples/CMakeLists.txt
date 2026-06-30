#   ____  _             _         _____                           _           
#  |  _ \| |_   _  __ _(_)_ __   | ____|_  ____ _ _ __ ___  _ __ | | ___  ___ 
#  | |_) | | | | |/ _` | | '_ \  |  _| \ \/ / _` | '_ ` _ \| '_ \| |/ _ \/ __|
#  |  __/| | |_| | (_| | | | | | | |___ >  < (_| | | | | | | |_) | |  __/\__ \
#  |_|   |_|\__,_|\__, |_|_| |_| |_____/_/\_\__,_|_| |_| |_| .__/|_|\___||___/
#                 |___/                                    |_|                
cmake_minimum_required(VERSION 3.20)

set(PLUGIN_EXAMPLES_DIR ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT DEFINED SRC_DIR)
  set(SRC_DIR ${PLUGIN_EXAMPLES_DIR}/../src)
endif()
if(NOT DEFINED USR_DIR)
  set(USR_DIR ${PLUGIN_EXAMPLES_DIR}/../usr)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(plugin_examples VERSION 0.1.0 LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  option(MADS_NO_DEPS_ONLY "Build only examples without external dependencies" OFF)

  include(FetchContent)

  if(NOT TARGET pugg)
    FetchContent_Declare(pugg
      GIT_REPOSITORY https://github.com/pbosetti/pugg.git
      GIT_TAG        1.1.0
      GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(pugg)
  endif()

  if(NOT TARGET nlohmann_json::nlohmann_json)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_Declare(nlohmann_json
      URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
  endif()

  if(NOT TARGET mads_plugin)
    add_library(mads_plugin INTERFACE)
    target_compile_features(mads_plugin INTERFACE cxx_std_17)
    target_include_directories(mads_plugin INTERFACE ${SRC_DIR})
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
endif()

if(APPLE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib;/usr/local/lib")
endif()

if(UNIX AND NOT APPLE)
  find_library(PLUGIN_UUID_LIB uuid)
  if(NOT PLUGIN_UUID_LIB)
    set(PLUGIN_UUID_LIB uuid)
  endif()
endif()

if(NOT MADS_NO_DEPS_ONLY)
  find_path(PLUGIN_MOSQUITTOPP_INCLUDE_DIR mosquittopp.h)
  find_library(PLUGIN_MOSQUITTOPP_LIBRARY mosquittopp)
  if(PLUGIN_MOSQUITTOPP_INCLUDE_DIR AND PLUGIN_MOSQUITTOPP_LIBRARY)
    set(PLUGIN_HAS_MOSQUITTOPP TRUE)
  else()
    message(STATUS "Skipping mqtt example: mosquittopp was not found")
  endif()
endif()

set(PLUGIN_EXAMPLE_TARGETS)

# MACROS #######################################################################
# Call: add_plugin(name [SRCS src1 src2 ...] [LIBS lib1 lib2 ...])
#       the source file ${PLUGIN_EXAMPLES_DIR}/plugin/<name>.cpp is implicitly
#       added.
macro(add_plugin name)
  set(multiValueArgs INCLUDES LIBS SRCS)
  cmake_parse_arguments(plugin "" "" "${multiValueArgs}" ${ARGN})
  set(plugin_source ${PLUGIN_EXAMPLES_DIR}/plugin/${name}.cpp)

  if(APPLE)
    add_executable(${name} ${plugin_source} ${plugin_SRCS})
    target_include_directories(${name}
      PRIVATE
        ${PLUGIN_EXAMPLES_DIR}
        ${plugin_INCLUDES}
    )
    target_link_libraries(${name} PRIVATE MADS::Plugin ${plugin_LIBS})
    target_compile_definitions(${name} PRIVATE PLUGIN_NAME="${name}")
    set_target_properties(${name} PROPERTIES ENABLE_EXPORTS TRUE)
    set(${name}_EXEC_TARGET ${name})
  else()
    add_library(${name} SHARED ${plugin_source} ${plugin_SRCS})
    target_include_directories(${name}
      PRIVATE
        ${PLUGIN_EXAMPLES_DIR}
        ${plugin_INCLUDES}
    )
    target_link_libraries(${name} PRIVATE MADS::Plugin ${plugin_LIBS})
    target_compile_definitions(${name} PRIVATE PLUGIN_NAME="${name}")

    add_executable(${name}_main ${plugin_source} ${plugin_SRCS})
    target_include_directories(${name}_main
      PRIVATE
        ${PLUGIN_EXAMPLES_DIR}
        ${plugin_INCLUDES}
    )
    target_link_libraries(${name}_main PRIVATE MADS::Plugin ${plugin_LIBS})
    target_compile_definitions(${name}_main PRIVATE PLUGIN_NAME="${name}")
    set_target_properties(${name}_main PROPERTIES OUTPUT_NAME ${name})
    set(${name}_EXEC_TARGET ${name}_main)
    list(APPEND PLUGIN_EXAMPLE_TARGETS ${name}_main)
  endif()

  set_target_properties(${name} PROPERTIES PREFIX "")
  set_target_properties(${name} PROPERTIES SUFFIX ".plugin")
  list(APPEND PLUGIN_EXAMPLE_TARGETS ${name})
endmacro()

macro(add_loader name)
  add_executable(${name} ${PLUGIN_EXAMPLES_DIR}/main/${name}.cpp)
  target_link_libraries(${name} PRIVATE MADS::Plugin)
  list(APPEND PLUGIN_EXAMPLE_TARGETS ${name})
endmacro()

# BUILD SETTINGS ###############################################################
add_loader(load_filter)
add_loader(load_source)
add_loader(load_sink)

# These plugins are always built and used for testing.
add_plugin(echoj)
add_plugin(clock)

# Add more plugin targets in examples/plugin/CMakeLists.txt.
add_subdirectory(${PLUGIN_EXAMPLES_DIR}/plugin)

# INSTALL ######################################################################
install(TARGETS ${PLUGIN_EXAMPLE_TARGETS}
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib
  ARCHIVE DESTINATION lib
)

# TESTING ######################################################################
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
endif()

add_test(NAME "echoj plugin exec" COMMAND $<TARGET_FILE:${echoj_EXEC_TARGET}>)
add_test(NAME "clock plugin exec" COMMAND $<TARGET_FILE:${clock_EXEC_TARGET}>)
add_test(
  NAME "load_filter echoj.plugin"
  COMMAND $<TARGET_FILE:load_filter> $<TARGET_FILE:echoj>
)
add_test(
  NAME "load_source clock.plugin"
  COMMAND $<TARGET_FILE:load_source> $<TARGET_FILE:clock>
)
