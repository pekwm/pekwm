bin_PROGRAMS = pekwm_sys
pekwm_sys_SOURCES = pekwm_sys.cc pekwm_sys.hh \
		    SysConfig.cc SysConfig.hh \
		    SysMonitorChange.cc SysMonitorChange.hh \
		    SysMonitorConfig.cc SysMonitorConfig.hh \
		    SysResources.cc SysResources.hh \
		    XSettings.cc XSettings.hh
pekwm_sys_CXXFLAGS = $(LIB_CFLAGS) $(libcurl_CFLAGS) $(libudev_CFLAGS) \
		     -I../lib
pekwm_sys_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a $(LIB_LIBS) \
		  $(libcurl_LIBS) $(libudev_LIBS)

EXTRA_DIST = CMakeLists.txt
