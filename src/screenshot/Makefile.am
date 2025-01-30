bin_PROGRAMS = pekwm_screenshot
pekwm_screenshot_SOURCES = pekwm_screenshot.cc
pekwm_screenshot_CXXFLAGS = -I../lib
pekwm_screenshot_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
pekwm_screenshot_LDFLAGS = $(X_LIBS) -lX11 $(xext_LIBS) $(libjpeg_LIBS) \
	$(libpng_LIBS) $(xpm_LIBS)
