add_executable(test_client test_client.cc)
set_target_properties(test_client PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)
target_include_directories(test_client PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_client ${X11_LIBRARIES})

add_executable(test_update_client_list test_update_client_list.cc)
set_target_properties(test_update_client_list PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)
target_include_directories(test_update_client_list PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_update_client_list ${X11_LIBRARIES})
