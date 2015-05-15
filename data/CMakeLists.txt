cmake_minimum_required(VERSION 2.8)

add_subdirectory(themes)
add_subdirectory(scripts)

install(FILES
  autoproperties
  autoproperties_typerules
  config
  config_system
  keys
  menu
  mouse
  mouse_system
  mouse_click
  mouse_sloppy
  start
  vars
  DESTINATION share/pekwm/)
