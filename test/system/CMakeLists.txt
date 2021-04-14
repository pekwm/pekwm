set(common_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src
                        ${X11_INCLUDE_DIR})
set(common_LIBRARIES ${X11_LIBRARIES})

if (ENABLE_SHAPE AND X11_Xshape_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xshape_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xshape_LIB})
endif (ENABLE_SHAPE AND X11_Xshape_FOUND)

if (ENABLE_XINERAMA AND X11_Xinerama_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xinerama_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xinerama_LIB})
endif (ENABLE_XINERAMA AND X11_Xinerama_FOUND)

if (ENABLE_XFT AND X11_Xft_FOUND AND FREETYPE_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xft_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xft_LIB} ${FREETYPE_LIBRARIES})
endif (ENABLE_XFT AND X11_Xft_FOUND AND FREETYPE_FOUND)

if (ENABLE_IMAGE_JPEG AND JPEG_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})
  set(common_LIBRARIES ${common_LIBRARIES} ${JPEG_LIBRARY})
endif (ENABLE_IMAGE_JPEG AND JPEG_FOUND)

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${PNG_INCLUDE_DIR})
  set(common_LIBRARIES ${common_LIBRARIES} ${PNG_LIBRARY_RELEASE})
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

if (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xpm_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xpm_LIB})
endif (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)

if (ENABLE_RANDR AND X11_Xrandr_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xrandr_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xrandr_LIB})
endif (ENABLE_RANDR AND X11_Xrandr_FOUND)

add_executable(test_client test_client.cc)
set_target_properties(test_client PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)
target_include_directories(test_client PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_client ${X11_LIBRARIES})

add_executable(test_transient_for test_transient_for.cc)
set_target_properties(test_transient_for PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)
target_include_directories(test_transient_for PUBLIC ${common_INCLUDE_DIR})
target_link_libraries(test_transient_for x11 util ${common_LIBRARIES})

add_executable(test_update_client_list test_update_client_list.cc)
set_target_properties(test_update_client_list PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)
target_include_directories(test_update_client_list PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_update_client_list ${X11_LIBRARIES})

add_executable(test_net_request_frame_extents test_net_request_frame_extents.cc)
set_target_properties(test_net_request_frame_extents PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)
target_include_directories(test_net_request_frame_extents PUBLIC ${X11_INCLUDE_DIR})
target_link_libraries(test_net_request_frame_extents ${X11_LIBRARIES})
