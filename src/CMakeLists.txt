cmake_minimum_required(VERSION 3.10)

configure_file(${CMAKE_SOURCE_DIR}/CMake/config.h.in
	       ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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
    ThemeGm.cc
    WORefMenu.cc
    WindowManager.cc
    WinLayouter.cc
    Workspaces.cc
    WorkspaceIndicator.cc
    WmUtil.cc)

add_library(wm STATIC ${wm_SOURCES})
target_include_directories(wm PUBLIC ${common_INCLUDE_DIRS})

add_executable(pekwm pekwm.cc pekwm_env.cc lib/Compat.cc)
target_compile_definitions(pekwm PUBLIC PEKWM_SH="${SH}")
target_include_directories(pekwm PUBLIC
			   ${PROJECT_BINARY_DIR}/src
			   ${PROJECT_SOURCE_DIR}/src/lib)
install(TARGETS pekwm DESTINATION bin)

add_executable(pekwm_wm pekwm_wm.cc pekwm_env.cc)
target_include_directories(pekwm_wm PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_wm wm tk lib ${common_LIBRARIES})
install(TARGETS pekwm_wm DESTINATION bin)

# Subdirectories
add_subdirectory(bg)
if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_subdirectory(cfg)
endif (CMAKE_BUILD_TYPE MATCHES Debug)
add_subdirectory(ctrl)
add_subdirectory(dialog)
add_subdirectory(lib)
add_subdirectory(panel)
if (ENABLE_IMAGE_PNG AND PNG_FOUND)
	add_subdirectory(screenshot)
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)
add_subdirectory(tk)
