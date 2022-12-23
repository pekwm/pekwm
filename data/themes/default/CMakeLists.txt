cmake_minimum_required(VERSION 3.5)

file(GLOB default_png_files "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

install(FILES
	theme
	${default_png_files}
	DESTINATION share/pekwm/themes/default)

add_subdirectory(backgrounds)
