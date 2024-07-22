#   ____  _             _         _                       _       
#  |  _ \| |_   _  __ _(_)_ __   | |_ __ _ _ __ __ _  ___| |_ ___ 
#  | |_) | | | | |/ _` | | '_ \  | __/ _` | '__/ _` |/ _ \ __/ __|
#  |  __/| | |_| | (_| | | | | | | || (_| | | | (_| |  __/ |_\__ \
#  |_|   |_|\__,_|\__, |_|_| |_|  \__\__,_|_|  \__, |\___|\__|___/
#                 |___/                        |___/              

# Add plugin targets here
# Use the syntax: 
#   add_plugin(<plugin_name> [SRCS <source_files>] [LIBS <libraries>])
# If the plugin has only one source file, that is the plugin_name.cpp file, 
# you can omit the SRCS argument:
#   add_plugin(plugin_name)
# Use the SRCS argument to specify additional source files.
# Use the LIBS argument to specify additional libraries that the plugin 
# depends on.

# PLUGINS ######################################################################
# These plugins are built unconditionally.

add_plugin(to_console)
add_plugin(random)
add_plugin(running_avg)
add_plugin(worker)
if(UNIX AND NOT APPLE)
  add_plugin(spawner LIBS uuid)
elseif(APPLE)
  add_plugin(spawner)
endif()

# CONDITIONAL PLUGINS ##########################################################
# These plugins are only built if the required dependencies are available.

if(NOT MADS_NO_DEPS_ONLY)
  if(NOT WIN32)
    # Serial port plugin is not supported on Windows
    add_plugin(serial_reader SRCS ${SRC_DIR}/serialport.cpp)
  endif()
  add_plugin(mqtt LIBS mosquittopp)
endif()