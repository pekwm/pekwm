cmake_minimum_required(VERSION 3.10)

add_executable(pekwm_cfg pekwm_cfg.cc)
target_include_directories(pekwm_cfg PUBLIC ${PROJECT_BINARY_DIR}/src)
target_link_libraries(pekwm_cfg lib ${common_LIBRARIES})
install(TARGETS pekwm_cfg DESTINATION bin)
