bin_PROGRAMS = pekwm_bg
pekwm_bg_SOURCES = pekwm_bg.cc
pekwm_bg_CXXFLAGS = -I../lib
pekwm_bg_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
pekwm_bg_LDFLAGS = $(X_LIBS) -lX11 $(xext_LIBS) $(libjpeg_LIBS) \
	$(libpng_LIBS) $(xpm_LIBS) $(xft_LIBS) \
	$(pango_LIBS) $(pangocairo_LIBS) $(pangoxft_LIBS)
