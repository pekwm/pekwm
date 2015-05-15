cmake_minimum_required(VERSION 2.8)

file(GLOB default_png_files "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

install(FILES
  theme
  ${default_png_files}
  DESTINATION share/pekwm/themes/default)
