bin_PROGRAMS = pekwm_panel
pekwm_panel_SOURCES = pekwm_panel.cc \
	../pekwm_env.cc \
	BarWidget.cc \
	ClientInfo.cc \
	ClientListWidget.cc \
	ExternalCommandData.cc \
	IconWidget.cc \
	PanelConfig.cc \
	PanelTheme.cc \
	PanelWidget.cc \
	SystrayWidget.cc \
	TextFormatter.cc \
	TextWidget.cc \
	VarData.cc \
	WidgetFactory.cc \
	WmState.cc
pekwm_panel_CXXFLAGS = -I../lib
pekwm_panel_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
pekwm_panel_LDFLAGS = $(X_LIBS) -lX11 $(xext_LIBS) $(libjpeg_LIBS) \
	$(libpng_LIBS) $(xpm_LIBS) $(xft_LIBS) \
	$(pango_LIBS) $(pangocairo_LIBS) $(pangoxft_LIBS)
