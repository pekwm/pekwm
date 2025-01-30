bin_PROGRAMS = pekwm pekwm_wm

pekwm_SOURCES = pekwm.cc ../pekwm_env.cc ../lib/Compat.cc
pekwm_CXXFLAGS = -I../lib -I..

pekwm_wm_SOURCES = pekwm_wm.cc \
	../pekwm_env.cc \
	ActionHandler.cc \
	ActionMenu.cc \
	AutoProperties.cc \
	Client.cc \
	ClientMgr.cc \
	CmdDialog.cc \
	Completer.cc \
	Config.cc \
	DockApp.cc \
	FocusToggleEventHandler.cc \
	Frame.cc \
	FrameListMenu.cc \
	Globals.cc \
	GroupingDragEventHandler.cc \
	Harbour.cc \
	InputDialog.cc \
	KeyGrabber.cc \
	KeyboardMoveResizeEventHandler.cc \
	ManagerWindows.cc \
	MenuHandler.cc \
	MoveEventHandler.cc \
	PDecor.cc \
	PMenu.cc \
	ResizeEventHandler.cc \
	SearchDialog.cc \
	StatusWindow.cc \
	ThemeGm.cc \
	WORefMenu.cc \
	WinLayouter.cc \
	WindowManager.cc \
	WmUtil.cc \
	WorkspaceIndicator.cc \
	Workspaces.cc
pekwm_wm_CXXFLAGS = -I../lib -I.. -DFEATURES=\"@PEKWM_FEATURES@\"
pekwm_wm_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
pekwm_wm_LDFLAGS = $(X_LIBS) -lX11 $(xext_LIBS) $(libjpeg_LIBS) \
	$(libpng_LIBS) $(xpm_LIBS) $(xft_LIBS) \
	$(pango_LIBS) $(pangocairo_LIBS) $(pangoxft_LIBS)
