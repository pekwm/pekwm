cmake_minimum_required(VERSION 3.10)

configure_file(${CMAKE_SOURCE_DIR}/CMake/config.h.in
	       ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(lib)
add_subdirectory(bg)
if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_subdirectory(cfg)
endif (CMAKE_BUILD_TYPE MATCHES Debug)
add_subdirectory(ctrl)
add_subdirectory(dialog)
add_subdirectory(panel)
add_subdirectory(sys)
add_subdirectory(tk)
if (ENABLE_IMAGE_PNG AND PNG_FOUND)
	add_subdirectory(screenshot)
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)
add_subdirectory(wm)
