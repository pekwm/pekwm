bin_PROGRAMS = pekwm_sys
pekwm_sys_SOURCES = pekwm_sys.cc \
	SysConfig.cc \
	SysResources.cc \
	XSettings.cc
pekwm_sys_CXXFLAGS = -I../lib
pekwm_sys_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
pekwm_sys_LDFLAGS = $(X_LIBS) -lX11 $(xext_LIBS) $(libcurl_LIBS)

