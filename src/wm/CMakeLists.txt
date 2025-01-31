cmake_minimum_required(VERSION 3.10)

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
target_include_directories(wm PUBLIC ${common_INCLUDE_DIRS}
			   ${PROJECT_BINARY_DIR}/src
			   ${PROJECT_SOURCE_DIR}/src)

add_executable(pekwm pekwm.cc ../pekwm_env.cc ../lib/Compat.cc)
target_compile_definitions(pekwm PUBLIC PEKWM_SH="${SH}")
target_include_directories(pekwm PUBLIC
			   ${PROJECT_BINARY_DIR}/src
			   ${PROJECT_SOURCE_DIR}/src
			   ${PROJECT_SOURCE_DIR}/src/lib)
install(TARGETS pekwm DESTINATION bin)

add_executable(pekwm_wm pekwm_wm.cc ../pekwm_env.cc)
target_include_directories(pekwm_wm PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_wm wm tk lib ${common_LIBRARIES})
install(TARGETS pekwm_wm DESTINATION bin)
