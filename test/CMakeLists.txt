set(common_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src
                        ${ICONV_INCLUDE_DIR} ${X11_INCLUDE_DIR})
set(common_LIBRARIES ${ICONV_LIBRARIES} ${X11_LIBRARIES})

if (X11_Xshape_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xshape_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xshape_LIB})
endif (X11_Xshape_FOUND)

if (X11_Xinerama_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xinerama_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xinerama_LIB})
endif (X11_Xinerama_FOUND)

if (X11_Xft_FOUND AND FREETYPE_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xft_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xft_LIB} ${FREETYPE_LIBRARIES})
endif (X11_Xft_FOUND AND FREETYPE_FOUND)

if (PNG_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${PNG_INCLUDE_DIR})
  set(common_LIBRARIES ${common_LIBRARIES} ${PNG_LIBRARY_RELEASE})
endif (PNG_FOUND)

if (X11_Xpm_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xpm_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xpm_LIB})
endif (X11_Xpm_FOUND)

if (X11_Xrandr_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xrandr_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xrandr_LIB})
endif (X11_Xrandr_FOUND)

add_executable(test_Util test_Util.cc)
add_test(Util test_Util)

target_include_directories(test_Util PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(test_Util libpekwm ${common_LIBRARIES})
