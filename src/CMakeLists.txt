cmake_minimum_required(VERSION 2.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(${CMAKE_SOURCE_DIR}/CMake/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(libpekwm_SOURCES
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
  x11.cc)
set(common_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/src ${ICONV_INCLUDE_DIR} ${X11_INCLUDE_DIR})
set(common_LIBRARIES ${ICONV_LIBRARIES} ${X11_LIBRARIES})

if (DEBUG)
  set(libpekwm_SOURCES ${libpekwm_SOURCES} Debug.cc)
endif (DEBUG)

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

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
  set(libpekwm_SOURCES ${libpekwm_SOURCES} PImageLoaderPng.cc)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${PNG_INCLUDE_DIR})
  set(common_LIBRARIES ${common_LIBRARIES} ${PNG_LIBRARY_RELEASE})
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

if (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)
  set(libpekwm_SOURCES ${libpekwm_SOURCES} PImageLoaderXpm.cc)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xpm_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xpm_LIB})
endif (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)

if (ENABLE_RANDR AND X11_Xrandr_FOUND)
  set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xrandr_INCLUDE_PATH})
  set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xrandr_LIB})
endif (ENABLE_RANDR AND X11_Xrandr_FOUND)

add_library(libpekwm STATIC ${libpekwm_SOURCES})
target_include_directories(libpekwm PUBLIC ${common_INCLUDE_DIRS})

set_target_properties(libpekwm PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)

add_executable(pekwm main.cc)
target_include_directories(pekwm PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm libpekwm ${common_LIBRARIES})

set_target_properties(pekwm PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON)

install(TARGETS pekwm DESTINATION bin)
