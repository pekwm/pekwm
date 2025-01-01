cmake_minimum_required(VERSION 3.10)

add_executable(pekwm_screenshot pekwm_screenshot.cc)
target_include_directories(pekwm_screenshot
			   PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_screenshot lib tk
		      ${common_LIBRARIES})
install(TARGETS pekwm_screenshot DESTINATION bin)
