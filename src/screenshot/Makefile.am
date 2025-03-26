if PEKWM_WITH_IMAGE_PNG
bin_PROGRAMS = pekwm_screenshot
pekwm_screenshot_SOURCES = pekwm_screenshot.cc
pekwm_screenshot_CXXFLAGS = $(LIB_CFLAGS) $(TK_CFLAGS) -I../lib
pekwm_screenshot_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a $(LIB_LIBS) $(TK_LIBS)
endif

EXTRA_DIST = CMakeLists.txt $(pekwm_screenshot_SOURCES)
