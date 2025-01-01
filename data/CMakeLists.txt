cmake_minimum_required(VERSION 3.10)

add_subdirectory(themes)
add_subdirectory(scripts)

install(FILES
	autoproperties
	autoproperties_typerules
	config
	config_system
	keys
	keys_inputdialog
	keys_menu
	keys_moveresize
	menu
	menu_windowmenu
	mouse
	mouse_system
	mouse_click
	mouse_sloppy
	panel
	start
	vars
	DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/pekwm/)
