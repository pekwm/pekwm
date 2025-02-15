cmake_minimum_required(VERSION 3.10)

add_executable(pekwm_panel
	       pekwm_panel.cc
	       ../pekwm_env.cc
	       BarWidget.cc
	       ClientInfo.cc
	       ClientListWidget.cc
	       ExternalCommandData.cc
	       IconWidget.cc
	       PanelConfig.cc
	       PanelTheme.cc
	       PanelWidget.cc
	       SystrayWidget.cc
	       TextFormatter.cc
	       TextWidget.cc
	       VarData.cc
	       WidgetFactory.cc
	       WmState.cc)
target_compile_definitions(pekwm_panel PUBLIC PEKWM_SH="${SH}")
target_include_directories(pekwm_panel PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_panel lib tk ${common_LIBRARIES})
install(TARGETS pekwm_panel DESTINATION bin)

add_executable(pekwm_panel_sysinfo
	       pekwm_panel_sysinfo.c)
if (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
target_link_libraries(pekwm_panel_sysinfo -lkstat)
endif (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
install(TARGETS pekwm_panel_sysinfo DESTINATION share/pekwm/scripts)
