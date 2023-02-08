add_executable(test_client test_client.cc)
target_include_directories(test_client PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_client ${X11_LIBRARIES})

add_executable(test_transient_for test_transient_for.cc)
target_include_directories(test_transient_for PUBLIC
			   ${PROJECT_SOURCE_DIR}/src
			   ${common_INCLUDE_DIRS})
target_link_libraries(test_transient_for lib tk ${common_LIBRARIES})

add_executable(test_update_client_list test_update_client_list.cc)
target_include_directories(test_update_client_list PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_update_client_list ${X11_LIBRARIES})

add_executable(test_net_request_frame_extents
	       test_net_request_frame_extents.cc)
target_include_directories(test_net_request_frame_extents
			   PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_net_request_frame_extents ${X11_LIBRARIES})

add_executable(test_systray
	       test_systray.cc)
target_include_directories(test_systray
			   PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_systray ${X11_LIBRARIES})
