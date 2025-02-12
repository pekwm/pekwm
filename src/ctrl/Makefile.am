bin_PROGRAMS = pekwm_ctrl
pekwm_ctrl_SOURCES = pekwm_ctrl.cc
pekwm_ctrl_CXXFLAGS = $(LIB_CFLAGS) $(libcurl_CFLAGS) -I../lib
pekwm_ctrl_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
pekwm_ctrl_LDFLAGS = $(LIB_LIBS) $(libcurl_LIBS)

EXTRA_DIST = CMakeLists.txt
