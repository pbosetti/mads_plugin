#   ____  _             _       
#  |  _ \| |_   _  __ _(_)_ __  
#  | |_) | | | | |/ _` | | '_ \ 
#  |  __/| | |_| | (_| | | | | |
#  |_|   |_|\__,_|\__, |_|_| |_|
#                 |___/         
cmake_minimum_required(VERSION 3.20)
project(plugin VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)

# PROJECT SETTINGS #############################################################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(USR_DIR ${CMAKE_CURRENT_LIST_DIR}/usr)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(CMAKE_INSTALL_PREFIX ${USR_DIR})
set(FETCHCONTENT_QUIET OFF)

# DEPENDENCIES #################################################################
include(FetchContent)
# pugg is for the plugin system
FetchContent_Declare(pugg 
  GIT_REPOSITORY https://github.com/pbosetti/pugg.git
  GIT_TAG        1.0.2
  GIT_SHALLOW    TRUE
)
# plugins deal with I/O as JSON objects
set(BUILD_TESTING OFF CACHE INTERNAL "")
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
FetchContent_Declare(restclient-cpp
  GIT_REPOSITORY https://github.com/mrtazz/restclient-cpp
  GIT_TAG        HEAD
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(pugg json restclient-cpp)

FetchContent_Declare(eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG        HEAD
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(gcem
  GIT_REPOSITORY https://github.com/kthohr/gcem.git
  GIT_TAG        HEAD
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(stats
  GIT_REPOSITORY https://github.com/kthohr/stats.git
  GIT_TAG        HEAD
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(pugg json eigen gcem stats)



# MACROS #######################################################################
# Call: add_plugin(name [SRCS src1 src2 ...] [LIBS lib1 lib2 ...])
#       the source file ${SRC_DIR}/plugin/<name>.cpp is implicitly added
macro(add_plugin name)
  # on MacOS only, plugins can be compiled as executables
  set(multiValueArgs LIBS SRCS)
  cmake_parse_arguments(plugin "" "" "${multiValueArgs}" ${ARGN})
  if (APPLE)
    add_executable(${name} ${SRC_DIR}/plugin/${name}.cpp ${plugin_SRCS})
    set_target_properties(${name} PROPERTIES ENABLE_EXPORTS TRUE)
    set(${name}_EXEC ${name}.plugin)
  else()
    add_library(${name} SHARED ${SRC_DIR}/plugin/${name}.cpp ${plugin_SRCS})
    add_executable(${name}_main ${SRC_DIR}/plugin/${name}.cpp ${plugin_SRCS})
    target_link_libraries(${name}_main PRIVATE pugg ${plugin_LIBS})
    set_target_properties(${name}_main PROPERTIES OUTPUT_NAME ${name})
    set(${name}_EXEC ${name})
    list(APPEND TARGET_LIST ${name}_main)
  endif()
  target_link_libraries(${name} PRIVATE pugg ${plugin_LIBS})
  set_target_properties(${name} PROPERTIES PREFIX "")
  set_target_properties(${name} PROPERTIES SUFFIX ".plugin")
  target_compile_definitions(${name} PRIVATE PLUGIN_NAME="${name}")
  list(APPEND TARGET_LIST ${name})
endmacro()

macro(add_loader name)
  add_executable(${name} ${SRC_DIR}/main/${name}.cpp)
  target_link_libraries(${name} PRIVATE pugg)
  list(APPEND TARGET_LIST ${name})
endmacro()


# BUILD SETTINGS ###############################################################
if (APPLE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib;/usr/local/lib")
endif()
include_directories(${json_SOURCE_DIR}/include)
include_directories(${restclient-cpp_SOURCE_DIR}/include)
link_directories(${restclient-cpp_BINARY_DIR})

include_directories(${gcem_SOURCE_DIR}/include)
include_directories(${stats_SOURCE_DIR}/include)
include_directories(${eigen_SOURCE_DIR})


# TARGETS ######################################################################
add_plugin(echoj)
add_plugin(clock)
add_plugin(to_console)
add_plugin(restway LIBS restclient-cpp)
add_plugin(random)

add_loader(load_filter)
add_loader(load_source)
add_loader(load_sink)


# INSTALL ######################################################################
set(CMAKE_INSTALL_PREFIX ${USR_DIR})
install(TARGETS ${TARGET_LIST}
  BUNDLE DESTINATION bin
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
)


# TESTING ######################################################################
include(CTest)
add_test(NAME "echoj plugin exec" COMMAND ${echoj_EXEC})
add_test(NAME "clock plugin exec" COMMAND ${clock_EXEC})
if (APPLE)
  add_test(NAME "load_filter echoj.plugin" COMMAND load_filter echoj.plugin)
  add_test(NAME "load_source clock.plugin" COMMAND load_source clock.plugin)
elseif(WIN32)
  add_test(NAME "load_filter echoj.plugin" COMMAND load_filter.exe echoj.plugin)
  add_test(NAME "load_source echoj.plugin" COMMAND load_source.exe clock.plugin)
else()
  add_test(NAME "load_filter echoj.plugin" COMMAND build/load_filter build/echoj.plugin WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  add_test(NAME "load_source echoj.plugin" COMMAND build/load_source build/clock.plugin WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endif()


# DOCUMENTATION ################################################################
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
  set(DOXYGEN_EXCLUDE_PATTERNS clock.cpp echo*.cpp twice.cpp webcam.cpp */main/*)
  doxygen_add_docs(Doxygen
    ${SRC_DIR} ${SRC_DIR}/plugin ${MARKDOWN_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doc
    CONFIG_FILE Doxyfile
  )
endif()