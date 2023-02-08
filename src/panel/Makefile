.POSIX:

MK       = ../../mk

include $(MK)/config.mk

PROGS = pekwm_panel

BASE_OBJS = ../pekwm_env.o ../Compat.o ../Charset.o ../Debug.o
CFG_PARSER_OBJS = ../CfgParser.o ../CfgParserKey.o ../CfgParserSource.o \
	../CfgParserVarExpander.o ../CfgParserVarExpanderX11.o

UTIL_OBJS = $(CFG_PARSER_OBJS) ../CfgUtil.o ../Observable.o ../RegexString.o \
	    ../String.o ../Util.o
IMAGE_LOADER_OBJS = ../PImageLoaderJpeg.o ../PImageLoaderPng.o \
		    ../PImageLoaderXpm.o
TEXTURE_OBJS = ../Action.o ../Color.o ../FontHandler.o ../ImageHandler.o \
	       ../PFont.o ../PFontX.o ../PFontX11.o ../PFontXmb.o \
	       ../PFontXft.o \
	       ../PImage.o ../PImageIcon.o ../PTexture.o \
	       ../PTexturePlain.o ../Render.o ../TextureHandler.o ../Theme.o \
	       ../ThemeGm.o
X11_OBJS = ../PWinObj.o ../X11.o ../X11Util.o ../X11App.o

PEKWM_PANEL_OBJS = pekwm_panel.o \
		   BarWidget.o ClientInfo.o ClientListWidget.o \
		   ExternalCommandData.o IconWidget.o PanelConfig.o \
		   PanelTheme.o PanelWidget.o SystrayWidget.o \
		   TextFormatter.o TextWidget.o VarData.o WidgetFactory.o \
		   WmState.o \
		   $(BASE_OBJS) $(UTIL_OBJS) \
		   $(X11_OBJS) $(TEXTURE_OBJS) $(IMAGE_LOADER_OBJS)

all: $(PROGS)

install: $(PROGS)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(PROGS) $(DESTDIR)$(PREFIX)/bin

pekwm_panel: $(PEKWM_PANEL_OBJS)
	$(LD) -o pekwm_panel $(PEKWM_PANEL_OBJS) $(LDFLAGS)

pekwm_wm: $(PEKWM_WM_OBJS)
	$(LD) -o pekwm_wm $(PEKWM_WM_OBJS) $(LDFLAGS)

clean:
	rm -f $(PROGS) *.o

.SUFFIXES: .cc .o
.cc.o:
	$(CXX) -I../ $(CXXFLAGS) -DPEKWM_SH="\"$(SH)\"" -c $<
