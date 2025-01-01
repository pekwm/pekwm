cmake_minimum_required(VERSION 3.10)

file(GLOB default_png_files "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

install(FILES
	${default_png_files}
	DESTINATION share/pekwm/themes/default/backgrounds)
