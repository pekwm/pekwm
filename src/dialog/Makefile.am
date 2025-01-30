bin_PROGRAMS = pekwm_dialog
pekwm_dialog_SOURCES = pekwm_dialog.cc ../pekwm_env.cc
pekwm_dialog_CXXFLAGS = -I../lib
pekwm_dialog_LDADD = ../tk/libpekwm_tk.a ../lib/libpekwm_lib.a
pekwm_dialog_LDFLAGS = $(X_LIBS) -lX11 $(xext_LIBS) $(libjpeg_LIBS) \
	$(libpng_LIBS) $(xpm_LIBS) $(xft_LIBS) \
	$(pango_LIBS) $(pangocairo_LIBS) $(pangoxft_LIBS)
