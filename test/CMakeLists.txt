add_executable(test_pekwm
	test_pekwm.cc
	../src/pekwm_env.cc)
add_test(NAME pekwm
	COMMAND test_pekwm
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_pekwm PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_pekwm wm texture x11 util ${common_LIBRARIES})

add_executable(test_pekwm_ctrl test_pekwm_ctrl.cc)
add_test(NAME pekwm_ctrl
	COMMAND test_pekwm_ctrl
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_pekwm_ctrl PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_pekwm_ctrl x11 util ${common_LIBRARIES})

add_executable(test_pekwm_panel
	test_pekwm_panel.cc
	../src/pekwm_env.cc)
add_test(NAME pekwm_panel
	COMMAND test_pekwm_panel
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_pekwm_panel PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_pekwm_panel wm texture x11 util ${common_LIBRARIES})

add_executable(test_util
	test_util.cc)
add_test(NAME util
	COMMAND test_util
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_util PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_util util x11 ${common_LIBRARIES})

add_subdirectory(system)
