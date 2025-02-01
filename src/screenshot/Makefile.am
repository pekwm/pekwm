if PEKWM_HAVE_IMAGE_PNG
	bin_PROGRAMS = pekwm_screenshot
	pekwm_screenshot_SOURCES = pekwm_screenshot.cc
	pekwm_screenshot_CXXFLAGS = -I../lib
	pekwm_screenshot_LDADD = ../lib/libpekwm_lib.a ../tk/libpekwm_tk.a
	pekwm_screenshot_LDFLAGS = $(LIB_LIBS)
endif

EXTRA_DIST = CMakeLists.txt $(pekwm_screenshot_SOURCES)
