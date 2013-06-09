AUTOMAKE_OPTIONS = foreign

CXXFLAGS = @CXXFLAGS@ -DSYSCONFDIR=\"$(sysconfdir)/pekwm\" -DDATADIR=\"$(datadir)\"

if DEBUG_INFRASTRUCTURE
debugsource = Debug.cc
else
debugsource =
endif

bin_PROGRAMS = pekwm
pekwm_SOURCES = $(debugsource)\
	Action.hh \
	ActionHandler.cc ActionHandler.hh \
	ActionMenu.cc ActionMenu.hh \
	AutoProperties.cc AutoProperties.hh \
	CfgParser.cc CfgParser.hh \
	CfgParserKey.cc CfgParserKey.hh \
	CfgParserSource.cc CfgParserSource.hh \
	Compat.cc Compat.hh \
	Completer.cc Completer.hh \
	Client.cc Client.hh \
	CmdDialog.cc CmdDialog.hh \
	Config.cc Config.hh \
	Debug.hh \
	DockApp.cc DockApp.hh \
	Exception.hh \
	Frame.cc Frame.hh \
	FontHandler.cc FontHandler.hh \
	FrameListMenu.cc FrameListMenu.hh \
	Harbour.cc Harbour.hh \
	Handler.hh \
	ImageHandler.cc ImageHandler.hh \
	InputDialog.cc InputDialog.hh \
	KeyGrabber.cc KeyGrabber.hh \
	ManagerWindows.cc ManagerWindows.hh \
	MenuHandler.cc MenuHandler.hh \
	Observer.hh \
	Observable.cc Observable.hh \
	PDecor.cc PDecor.hh \
	PFont.cc PFont.hh \
	PMenu.cc PMenu.hh \
	PTexture.hh \
	PTexturePlain.cc PTexturePlain.hh \
	PWinObj.cc PWinObj.hh \
	PWinObjReference.cc PWinObjReference.hh \
	PImage.cc PImage.hh \
	PImageIcon.cc PImageIcon.hh \
	PImageLoader.hh \
	PImageLoaderJpeg.cc PImageLoaderJpeg.hh \
	PImageLoaderPng.cc PImageLoaderPng.hh \
	PImageLoaderXpm.cc PImageLoaderXpm.hh \
	PixmapHandler.cc PixmapHandler.hh \
	RegexString.cc RegexString.hh \
	ParseUtil.hh \
	ScreenResources.cc ScreenResources.hh \
	StatusWindow.cc StatusWindow.hh \
	SearchDialog.cc SearchDialog.hh \
	Theme.cc Theme.hh \
	TextureHandler.cc TextureHandler.hh \
	Timer.hh Types.hh \
	Util.cc Util.hh \
	WORefMenu.cc WORefMenu.hh \
	WindowManager.cc WindowManager.hh \
	WinLayouter.cc \
	Workspaces.cc Workspaces.hh \
	WorkspaceIndicator.cc WorkspaceIndicator.hh \
	x11.cc x11.hh \
	main.cc \
	pekwm.hh

distclean-local:
	rm -f *\~ .\#*

