add_executable(test_pekwm
	test_pekwm.cc
	../src/pekwm_env.cc)
add_test(NAME pekwm
	COMMAND test_pekwm
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_pekwm PUBLIC
			   ${PROJECT_SOURCE_DIR}/src
			   ${common_INCLUDE_DIRS})
target_link_libraries(test_pekwm wm tk lib ${common_LIBRARIES})

add_executable(test_pekwm_ctrl test_pekwm_ctrl.cc)
add_test(NAME pekwm_ctrl
	COMMAND test_pekwm_ctrl
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_pekwm_ctrl PUBLIC
			   ${PROJECT_SOURCE_DIR}/src
			   ${common_INCLUDE_DIRS})
target_link_libraries(test_pekwm_ctrl lib ${common_LIBRARIES})

add_executable(test_pekwm_panel
	test_pekwm_panel.cc
	../src/pekwm_env.cc)
add_test(NAME pekwm_panel
	COMMAND test_pekwm_panel
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_pekwm_panel PUBLIC
			   ${PROJECT_SOURCE_DIR}/src
			   ${common_INCLUDE_DIRS})
target_link_libraries(test_pekwm_panel wm tk lib ${common_LIBRARIES})

add_executable(test_util
	test_util.cc)
add_test(NAME util
	COMMAND test_util
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/)
target_include_directories(test_util PUBLIC
			   ${PROJECT_SOURCE_DIR}/src
			   ${common_INCLUDE_DIRS})
target_link_libraries(test_util lib ${common_LIBRARIES})

add_subdirectory(system)
