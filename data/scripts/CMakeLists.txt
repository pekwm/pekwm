cmake_minimum_required(VERSION 2.8)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_themeset.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/pekwm_themeset.sh @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pekwm_ws_menu.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/pekwm_ws_menu.sh @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/pekwm_ws_menu.sh
  ${CMAKE_CURRENT_BINARY_DIR}/pekwm_themeset.sh
  pekwm_screenshot.sh
  DESTINATION share/pekwm/scripts
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)