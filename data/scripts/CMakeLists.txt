cmake_minimum_required(VERSION 3.10)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_audio_ctrl.sh.in
	       ${CMAKE_CURRENT_BINARY_DIR}/pekwm_audio_ctrl.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_lock.sh.in
	       ${CMAKE_CURRENT_BINARY_DIR}/pekwm_lock.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_themeset.sh.in
	       ${CMAKE_CURRENT_BINARY_DIR}/pekwm_themeset.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_ws_menu.sh.in
	       ${CMAKE_CURRENT_BINARY_DIR}/pekwm_ws_menu.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_toggle_kbd_layout.sh.in
	       ${CMAKE_CURRENT_BINARY_DIR}/pekwm_toggle_kbd_layout.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_panel_battery.sh.in
	       ${CMAKE_CURRENT_BINARY_DIR}/pekwm_panel_battery.sh @ONLY)


install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/pekwm_audio_ctrl.sh
	${CMAKE_CURRENT_BINARY_DIR}/pekwm_lock.sh
	${CMAKE_CURRENT_BINARY_DIR}/pekwm_ws_menu.sh
	${CMAKE_CURRENT_BINARY_DIR}/pekwm_themeset.sh
	${CMAKE_CURRENT_BINARY_DIR}/pekwm_toggle_kbd_layout.sh
	${CMAKE_CURRENT_BINARY_DIR}/pekwm_panel_battery.sh
	DESTINATION share/pekwm/scripts
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_theme.in
	       ${CMAKE_CURRENT_BINARY_DIR}/pekwm_theme @ONLY)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/pekwm_theme
	DESTINATION bin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
