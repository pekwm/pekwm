set(common_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src
                        ${ICONV_INCLUDE_DIR} ${X11_INCLUDE_DIR})
set(common_LIBRARIES ${ICONV_LIBRARIES} ${X11_LIBRARIES})

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

add_executable(test_Action test_Action.cc)
add_test(Action test_Action)
target_include_directories(test_Action PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_Action wm texture util ${common_LIBRARIES})

add_executable(test_Config test_Config.cc)
add_test(Config test_Config)
target_include_directories(test_Config PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_Config wm texture util ${common_LIBRARIES})

add_executable(test_Frame test_Frame.cc)
add_test(Frame test_Frame)
target_include_directories(test_Frame PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_Frame wm texture util ${common_LIBRARIES})

add_executable(test_Util test_Util.cc)
add_test(Util test_Util)
target_include_directories(test_Util PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_Util util ${common_LIBRARIES})

add_executable(test_x11 test_x11.cc)
add_test(x11 test_x11)
target_include_directories(test_x11 PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_x11 wm texture util ${common_LIBRARIES})

add_subdirectory(system)
