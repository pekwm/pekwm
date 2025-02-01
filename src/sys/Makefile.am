bin_PROGRAMS = pekwm_sys
pekwm_sys_SOURCES = pekwm_sys.cc \
		    SysConfig.cc SysConfig.hh \
		    SysResources.cc SysResources.hh \
		    XSettings.cc XSettings.hh
pekwm_sys_CXXFLAGS = -I../lib
pekwm_sys_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
pekwm_sys_LDFLAGS = $(LIB_LIBS) $(libcurl_LIBS)

EXTRA_DIST = CMakeLists.txt
