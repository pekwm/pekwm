cmake_minimum_required(VERSION 3.5)

configure_file(${CMAKE_SOURCE_DIR}/CMake/config.h.in
	       ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(util_SOURCES
    CfgParser.cc
    CfgParserKey.cc
    CfgParserSource.cc
    CfgParserVarExpander.cc
    CfgParserVarExpanderX11.cc
    CfgUtil.cc
    Charset.cc
    Compat.cc
    Debug.cc
    Observable.cc
    RegexString.cc
    String.cc
    Util.cc)

set(x11_SOURCES
    PWinObj.cc
    X11.cc
    X11Util.cc
    X11App.cc)

set(texture_SOURCES
    Action.cc
    Color.cc
    FontHandler.cc
    ImageHandler.cc
    PFont.cc
    PFontX.cc
    PFontX11.cc
    PFontXmb.cc
    PImage.cc
    PImageIcon.cc
    PTexture.cc
    PTexturePlain.cc
    Render.cc
    TextureHandler.cc
    Theme.cc
    ThemeGm.cc)

if (PEKWM_HAVE_XFT)
	set(texture_SOURCES ${texture_SOURCES} PFontXft.cc)
endif (PEKWM_HAVE_XFT)

if (PEKWM_HAVE_PANGO_CAIRO OR PEKWM_HAVE_PANGO_XFT)
	set(texture_SOURCES ${texture_SOURCES} PFontPango.cc)
endif (PEKWM_HAVE_PANGO_CAIRO OR PEKWM_HAVE_PANGO_XFT)

if (PEKWM_HAVE_PANGO_CAIRO)
	set(texture_SOURCES ${texture_SOURCES} PFontPangoCairo.cc)
endif (PEKWM_HAVE_PANGO_CAIRO)

if (PEKWM_HAVE_PANGO_XFT)
	set(texture_SOURCES ${texture_SOURCES} PFontPangoXft.cc)
endif (PEKWM_HAVE_PANGO_XFT)

if (ENABLE_IMAGE_JPEG AND JPEG_FOUND)
	set(texture_SOURCES ${texture_SOURCES} PImageLoaderJpeg.cc)
endif (ENABLE_IMAGE_JPEG AND JPEG_FOUND)

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
	set(texture_SOURCES ${texture_SOURCES} PImageLoaderPng.cc)
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

if (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)
	set(texture_SOURCES ${texture_SOURCES} PImageLoaderXpm.cc)
endif (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)

set(wm_SOURCES
    ActionHandler.cc
    ActionMenu.cc
    AutoProperties.cc
    Completer.cc
    Client.cc
    ClientMgr.cc
    CmdDialog.cc
    Config.cc
    DockApp.cc
    FocusToggleEventHandler.cc
    Frame.cc
    FrameListMenu.cc
    Globals.cc
    GroupingDragEventHandler.cc
    Harbour.cc
    InputDialog.cc
    KeyGrabber.cc
    KeyboardMoveResizeEventHandler.cc
    ManagerWindows.cc
    MenuHandler.cc
    MoveEventHandler.cc
    PDecor.cc
    PMenu.cc
    ResizeEventHandler.cc
    StatusWindow.cc
    SearchDialog.cc
    WORefMenu.cc
    WindowManager.cc
    WinLayouter.cc
    Workspaces.cc
    WorkspaceIndicator.cc
    WmUtil.cc)

add_library(util STATIC ${util_SOURCES})
target_include_directories(util PUBLIC ${common_INCLUDE_DIRS})
target_compile_definitions(util PUBLIC PEKWM_SH="${SH}")

add_library(x11 STATIC ${x11_SOURCES})
target_include_directories(x11 PUBLIC ${common_INCLUDE_DIRS})

add_library(texture STATIC ${texture_SOURCES})
target_include_directories(texture PUBLIC ${common_INCLUDE_DIRS})

add_library(wm STATIC ${wm_SOURCES})
target_include_directories(wm PUBLIC ${common_INCLUDE_DIRS})

add_executable(pekwm pekwm.cc Compat.cc)
target_compile_definitions(pekwm PUBLIC PEKWM_SH="${SH}")
target_include_directories(pekwm PUBLIC ${PROJECT_BINARY_DIR}/src)
install(TARGETS pekwm DESTINATION bin)

add_executable(pekwm_bg pekwm_bg.cc)
target_include_directories(pekwm_bg PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_bg texture x11 util ${common_LIBRARIES})
install(TARGETS pekwm_bg DESTINATION bin)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_executable(pekwm_cfg pekwm_cfg.cc)
	target_include_directories(pekwm_cfg PUBLIC ${PROJECT_BINARY_DIR}/src)
	target_link_libraries(pekwm_cfg util x11 ${common_LIBRARIES})
	install(TARGETS pekwm_cfg DESTINATION bin)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

add_executable(pekwm_dialog pekwm_dialog.cc)
target_include_directories(pekwm_dialog PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_dialog texture x11 util ${common_LIBRARIES})
install(TARGETS pekwm_dialog DESTINATION bin)

add_executable(pekwm_ctrl pekwm_ctrl.cc)
target_include_directories(pekwm_ctrl PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_ctrl x11 util ${common_LIBRARIES})
install(TARGETS pekwm_ctrl DESTINATION bin)

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
	add_executable(pekwm_screenshot pekwm_screenshot.cc)
	target_include_directories(pekwm_screenshot
				   PUBLIC ${common_INCLUDE_DIRS})
	target_link_libraries(pekwm_screenshot texture x11 util
			      ${common_LIBRARIES})
	install(TARGETS pekwm_screenshot DESTINATION bin)
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

add_executable(pekwm_wm pekwm_wm.cc)
target_include_directories(pekwm_wm PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_wm wm texture x11 util ${common_LIBRARIES})
install(TARGETS pekwm_wm DESTINATION bin)

# Subdirectories
add_subdirectory(panel)
