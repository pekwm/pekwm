cmake_minimum_required(VERSION 2.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(${CMAKE_SOURCE_DIR}/CMake/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(pekwm_SOURCES
  ActionHandler.cc
  ActionMenu.cc
  AutoProperties.cc
  CfgParser.cc
  CfgParserKey.cc
  CfgParserSource.cc
  Compat.cc
  Completer.cc
  Client.cc
  CmdDialog.cc
  Config.cc
  DockApp.cc
  Frame.cc
  FontHandler.cc
  FrameListMenu.cc
  Harbour.cc
  ImageHandler.cc
  InputDialog.cc
  KeyGrabber.cc
  LayouterTiling.cc
  ManagerWindows.cc
  MenuHandler.cc
  Observable.cc
  PDecor.cc
  PFont.cc
  PMenu.cc
  PTexturePlain.cc
  PWinObj.cc
  PWinObjReference.cc
  PImage.cc
  PImageIcon.cc
  RegexString.cc
  StatusWindow.cc
  SearchDialog.cc
  Theme.cc
  TextureHandler.cc
  Util.cc
  WORefMenu.cc
  WindowManager.cc
  WinLayouter.cc
  Workspaces.cc
  WorkspaceIndicator.cc
  x11.cc
  main.cc)
set(pekwm_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/src ${ICONV_INCLUDE_DIR} ${X11_INCLUDE_DIR})
set(pekwm_LIBRARIES ${ICONV_LIBRARIES} ${X11_LIBRARIES})

if (DEBUG)
  set(pekwm_SOURCES ${pekwm_SOURCES} Debug.cc)
endif (DEBUG)

if (X11_Xshape_FOUND)
  set(pekwm_INCLUDE_DIRS ${pekwm_INCLUDE_DIRS} ${X11_Xshape_INCLUDE_PATH})
  set(pekwm_LIBRARIES ${pekwm_LIBRARIES} ${X11_Xshape_LIB})
endif (X11_Xshape_FOUND)

if (X11_Xinerama_FOUND)
  set(pekwm_INCLUDE_DIRS ${pekwm_INCLUDE_DIRS} ${X11_Xinerama_INCLUDE_PATH})
  set(pekwm_LIBRARIES ${pekwm_LIBRARIES} ${X11_Xinerama_LIB})
endif (X11_Xinerama_FOUND)

if (X11_Xft_FOUND AND FREETYPE_FOUND)
  set(pekwm_INCLUDE_DIRS ${pekwm_INCLUDE_DIRS} ${X11_Xft_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS})
  set(pekwm_LIBRARIES ${pekwm_LIBRARIES} ${X11_Xft_LIB} ${FREETYPE_LIBRARIES})
endif (X11_Xft_FOUND AND FREETYPE_FOUND)

if (JPEG_FOUND)
  set(pekwm_SOURCES ${pekwm_SOURCES} PImageLoaderJpeg.cc)
  set(pekwm_INCLUDE_DIRS ${pekwm_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})
  set(pekwm_LIBRARIES ${pekwm_LIBRARIES} ${JPEG_LIBRARY})
endif (JPEG_FOUND)

if (PNG_FOUND)
  set(pekwm_SOURCES ${pekwm_SOURCES} PImageLoaderPng.cc)
  set(pekwm_INCLUDE_DIRS ${pekwm_INCLUDE_DIRS} ${PNG_INCLUDE_DIR})
  set(pekwm_LIBRARIES ${pekwm_LIBRARIES} ${PNG_LIBRARY_RELEASE})
endif (PNG_FOUND)

if (X11_Xpm_FOUND)
  set(pekwm_SOURCES ${pekwm_SOURCES} PImageLoaderXpm.cc)
  set(pekwm_INCLUDE_DIRS ${pekwm_INCLUDE_DIRS} ${X11_Xpm_INCLUDE_PATH})
  set(pekwm_LIBRARIES ${pekwm_LIBRARIES} ${X11_Xpm_LIB})
endif (X11_Xpm_FOUND)

if (X11_Xrandr_FOUND)
  set(pekwm_INCLUDE_DIRS ${pekwm_INCLUDE_DIRS} ${X11_Xrandr_INCLUDE_PATH})
  set(pekwm_LIBRARIES ${pekwm_LIBRARIES} ${X11_Xrandr_LIB})
endif (X11_Xrandr_FOUND)

add_executable(pekwm ${pekwm_SOURCES})
target_include_directories(pekwm PUBLIC ${pekwm_INCLUDE_DIRS})
target_link_libraries(pekwm ${pekwm_LIBRARIES})

# Use when CMake 3.1 is available/required
#set_target_properties(pekwm PROPERTIES
#  CXX_STANDARD 11
#  CXX_STANDARD_REQUIRED ON)

install(TARGETS pekwm DESTINATION bin)
