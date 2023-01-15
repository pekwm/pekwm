.POSIX:

MK       = ../mk

include $(MK)/config.mk

PROGS = pekwm pekwm_bg pekwm_cfg pekwm_ctrl pekwm_dialog \
	pekwm_screenshot pekwm_wm

BASE_OBJS = Compat.o Charset.o Debug.o
CFG_PARSER_OBJS = CfgParser.o CfgParserKey.o CfgParserSource.o \
	CfgParserVarExpander.o

UTIL_OBJS = $(CFG_PARSER_OBJS) CfgUtil.o Observable.o RegexString.o String.o \
	    Util.o
IMAGE_LOADER_OBJS = PImageLoaderJpeg.o PImageLoaderPng.o PImageLoaderXpm.o
TEXTURE_OBJS = Action.o Color.o FontHandler.o ImageHandler.o PFont.o \
	       PFontX11.o PFontXmb.o PFontXft.o \
	       PImage.o PImageIcon.o PTexture.o PTexturePlain.o Render.o \
	       TextureHandler.o Theme.o ThemeGm.o
X11_OBJS = PWinObj.o X11.o X11Util.o X11App.o
WM_OBJS = ActionHandler.o ActionMenu.o AutoProperties.o Completer.o \
	  Client.o ClientMgr.o CmdDialog.o Config.o DockApp.o \
	  FocusToggleEventHandler.o Frame.o FrameListMenu.o Globals.o \
	  GroupingDragEventHandler.o Harbour.o InputDialog.o KeyGrabber.o \
	  KeyboardMoveResizeEventHandler.o ManagerWindows.o MenuHandler.o \
	  MoveEventHandler.o PDecor.o PMenu.o ResizeEventHandler.o \
	  StatusWindow.o SearchDialog.o WORefMenu.o WindowManager.o \
	  WinLayouter.o Workspaces.o WorkspaceIndicator.o WmUtil.o

PEKWM_OBJS = pekwm.o Compat.o
PEKWM_BG_OBJS = pekwm_bg.o $(BASE_OBJS) $(CFG_PARSER_OBJS)  \
		String.o Util.o X11.o Color.o \
		ImageHandler.o TextureHandler.o PTexture.o PTexturePlain.o \
		Render.o PImage.o $(IMAGE_LOADER_OBJS)
PEKWM_CFG_OBJS = pekwm_cfg.o $(BASE_OBJS) $(CFG_PARSER_OBJS) \
	String.o Util.o X11.o
PEKWM_CTRL_OBJS = pekwm_ctrl.o $(BASE_OBJS) $(CFG_PARSER_OBJS) \
		  RegexString.o String.o Util.o X11.o
PEKWM_DIALOG_OBJS = pekwm_dialog.o $(BASE_OBJS) $(UTIL_OBJS) \
		    X11.o X11Util.o X11App.o PWinObj.o \
		    $(TEXTURE_OBJS) $(IMAGE_LOADER_OBJS)
PEKWM_SCREENSHOT_OBJS = pekwm_screenshot.o $(BASE_OBJS) String.o Util.o \
			$(CFG_PARSER_OBJS) \
			PImage.o Render.o $(IMAGE_LOADER_OBJS) X11.o ImageHandler.o
PEKWM_WM_OBJS = pekwm_wm.o $(BASE_OBJS) $(UTIL_OBJS) $(IMAGE_LOADER_OBJS) \
		$(TEXTURE_OBJS) $(X11_OBJS) $(WM_OBJS)

all: $(PROGS)
	( cd panel; $(MAKE) all )

install: $(PROGS)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(PROGS) $(DESTDIR)$(PREFIX)/bin

pekwm: $(PEKWM_OBJS)
	$(LD) -o pekwm $(PEKWM_OBJS) $(LDFLAGS)

pekwm_bg: $(PEKWM_BG_OBJS)
	$(LD) -o pekwm_bg $(PEKWM_BG_OBJS) $(LDFLAGS)

pekwm_cfg: $(PEKWM_CFG_OBJS)
	$(LD) -o pekwm_cfg $(PEKWM_CFG_OBJS) $(LDFLAGS)

pekwm_ctrl: $(PEKWM_CTRL_OBJS)
	$(LD) -o pekwm_ctrl $(PEKWM_CTRL_OBJS) $(LDFLAGS)

pekwm_dialog: $(PEKWM_DIALOG_OBJS)
	$(LD) -o pekwm_dialog $(PEKWM_DIALOG_OBJS) $(LDFLAGS)

pekwm_screenshot: $(PEKWM_SCREENSHOT_OBJS)
	$(LD) -o pekwm_screenshot $(PEKWM_SCREENSHOT_OBJS) $(LDFLAGS)

pekwm_wm: $(PEKWM_WM_OBJS)
	$(LD) -o pekwm_wm $(PEKWM_WM_OBJS) $(LDFLAGS)

clean:
	rm -f $(PROGS) *.o

.SUFFIXES: .cc .o
.cc.o:
	$(CXX) $(CXXFLAGS) -DPEKWM_SH="\"$(SH)\"" -c $<
