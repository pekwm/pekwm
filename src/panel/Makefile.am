bin_PROGRAMS = pekwm_panel
pekwm_panel_SOURCES = pekwm_panel.cc pekwm_panel.hh \
		      ../pekwm_env.cc \
		      BarWidget.cc BarWidget.hh \
		      ClientInfo.cc ClientInfo.hh \
		      ClientListWidget.cc ClientListWidget.hh \
		      DateTimeWidget.hh \
		      ExternalCommandData.cc ExternalCommandData.hh \
		      IconWidget.cc IconWidget.hh \
		      PanelAction.hh \
		      PanelConfig.cc PanelConfig.hh \
		      PanelTheme.cc PanelTheme.hh \
		      PanelWidget.cc PanelWidget.hh \
		      SystrayWidget.cc SystrayWidget.hh \
		      TextFormatter.cc TextFormatter.hh \
		      TextWidget.cc TextWidget.hh \
		      VarData.cc VarData.hh \
		      WidgetFactory.cc WidgetFactory.hh \
		      WmState.cc WmState.hh
pekwm_panel_CXXFLAGS = $(LIB_CFLAGS) $(TK_CFLAGS) -I../lib
pekwm_panel_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a $(LIB_LIBS) $(TK_LIBS)

scriptsdir = $(pkgdatadir)/scripts
scripts_PROGRAMS =  pekwm_panel_sysinfo
pekwm_panel_sysinfo_SOURCES = pekwm_panel_sysinfo.c
if PEKWM_HOST_SUNOS
pekwm_panel_sysinfo_LDFLAGS = -lkstat
endif

EXTRA_DIST = CMakeLists.txt
