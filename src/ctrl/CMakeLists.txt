cmake_minimum_required(VERSION 3.10)

add_executable(pekwm_ctrl pekwm_ctrl.cc)
target_include_directories(pekwm_ctrl PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_ctrl lib tk ${common_LIBRARIES})
install(TARGETS pekwm_ctrl DESTINATION bin)
