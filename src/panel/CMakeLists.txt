cmake_minimum_required(VERSION 3.5)

add_executable(pekwm_panel
	       pekwm_panel.cc
	       BarWidget.cc
	       ClientInfo.cc
	       ClientListWidget.cc
	       ExternalCommandData.cc
	       IconWidget.cc
	       PanelConfig.cc
	       PanelTheme.cc
	       PanelWidget.cc
	       TextFormatter.cc
	       TextWidget.cc
	       VarData.cc
	       WidgetFactory.cc
	       WmState.cc)
target_include_directories(pekwm_panel PUBLIC ${common_INCLUDE_DIRS})
target_link_libraries(pekwm_panel texture x11 util ${common_LIBRARIES})
install(TARGETS pekwm_panel DESTINATION bin)
