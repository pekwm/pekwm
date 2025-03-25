bin_PROGRAMS = pekwm_sys
pekwm_sys_SOURCES = pekwm_sys.cc \
		    SysConfig.cc SysConfig.hh \
		    SysResources.cc SysResources.hh \
		    XSettings.cc XSettings.hh
pekwm_sys_CXXFLAGS = $(LIB_CFLAGS) $(libcurl_CFLAGS) -I../lib
pekwm_sys_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a $(LIB_LIBS) $(libcurl_LIBS)

EXTRA_DIST = CMakeLists.txt
