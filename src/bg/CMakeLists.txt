cmake_minimum_required(VERSION 3.10)

add_executable(pekwm_bg pekwm_bg.cc)
target_include_directories(pekwm_bg PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_bg tk lib ${common_LIBRARIES})
install(TARGETS pekwm_bg DESTINATION bin)
