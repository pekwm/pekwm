cmake_minimum_required(VERSION 3.10)

add_executable(pekwm_sys
	       pekwm_sys.cc
	       SysConfig.cc
	       SysResources.cc
	       SysMonitorChange.cc
	       SysMonitorConfig.cc
	       XSettings.cc)
target_include_directories(pekwm_sys PUBLIC ${common_INCLUDE_DIRS}
					    ${LIBUDEV_INCLUDE_DIRS})
target_link_libraries(pekwm_sys tk lib ${common_LIBRARIES}
				${LIBUDEV_LINK_LIBRARIES})
install(TARGETS pekwm_sys DESTINATION bin)
