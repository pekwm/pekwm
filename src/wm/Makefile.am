bin_PROGRAMS = pekwm pekwm_wm

pekwm_SOURCES = pekwm.cc pekwm.hh ../pekwm_env.cc ../lib/Compat.cc
pekwm_CXXFLAGS = -I../lib -I..

pekwm_wm_SOURCES = pekwm_wm.cc \
	../pekwm_env.cc \
	ActionHandler.cc ActionHandler.hh \
	ActionMenu.cc ActionMenu.hh \
	AppCtrl.hh \
	AutoProperties.cc AutoProperties.hh \
	Client.cc Client.hh \
	ClientMgr.cc ClientMgr.hh \
	CmdDialog.cc CmdDialog.hh \
	Completer.cc Completer.hh \
	Config.cc Config.hh \
	DockApp.cc DockApp.hh \
	EventLoop.hh \
	EventHandler.hh \
	FocusToggleEventHandler.cc FocusToggleEventHandler.hh \
	Frame.cc Frame.hh \
	FrameListMenu.cc FrameListMenu.hh \
	Globals.cc \
	GroupingDragEventHandler.cc GroupingDragEventHandler.hh \
	Harbour.cc Harbour.hh \
	InputDialog.cc InputDialog.hh \
	KeyGrabber.cc KeyGrabber.hh \
	KeyboardMoveResizeEventHandler.cc KeyboardMoveResizeEventHandler.hh \
	ManagerWindows.cc ManagerWindows.hh \
	MenuHandler.cc MenuHandler.hh \
	MoveEventHandler.cc MoveEventHandler.hh \
	PDecor.cc PDecor.hh \
	PMenu.cc PMenu.hh \
	PWinObjReference.hh \
	ResizeEventHandler.cc ResizeEventHandler.hh \
	SearchDialog.cc SearchDialog.hh \
	StatusWindow.cc StatusWindow.hh \
	ThemeGm.cc ThemeGm.hh \
	WORefMenu.cc WORefMenu.hh \
	WinLayouter.cc WinLayouter.hh \
	WindowManager.cc WindowManager.hh \
	WmUtil.cc WmUtil.hh \
	WorkspaceIndicator.cc WorkspaceIndicator.hh \
	Workspaces.cc Workspaces.hh
pekwm_wm_CXXFLAGS = $(LIB_CFLAGS) $(TK_CFLAGS) -I../lib -I.. \
	-DFEATURES="\"@PEKWM_FEATURES@\""
pekwm_wm_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a $(LIB_LIBS) $(TK_LIBS)

EXTRA_DIST = CMakeLists.txt
