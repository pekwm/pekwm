cmake_minimum_required(VERSION 3.5)

add_executable(pekwm_show pekwm_show.cc ../pekwm_env.cc)
target_include_directories(pekwm_show PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_show tk lib ${common_LIBRARIES})
install(TARGETS pekwm_show DESTINATION bin)
