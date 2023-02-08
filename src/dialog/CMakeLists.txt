cmake_minimum_required(VERSION 3.5)

add_executable(pekwm_dialog pekwm_dialog.cc ../pekwm_env.cc)
target_include_directories(pekwm_dialog PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_dialog tk lib ${common_LIBRARIES})
install(TARGETS pekwm_dialog DESTINATION bin)
