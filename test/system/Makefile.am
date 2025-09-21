noinst_PROGRAMS = test_client \
		  test_net_request_frame_extents \
		  test_systray \
		  test_transient_for \
		  test_update_client_list

test_client_SOURCES = test_client.cc test_util.hh
test_client_CXXFLAGS = $(LIB_CFLAGS)
test_client_LDADD = ../../src/lib/libpekwm_lib.a $(LIB_LIBS)

test_net_request_frame_extents_SOURCES = test_net_request_frame_extents.cc
test_net_request_frame_extents_CXXFLAGS = $(LIB_CFLAGS)
test_net_request_frame_extents_LDADD = $(LIB_LIBS)

test_systray_SOURCES = test_systray.cc
test_systray_CXXFLAGS = $(LIB_CFLAGS) -I../../src/lib
test_systray_LDADD = $(LIB_LIBS)

test_transient_for_SOURCES = test_transient_for.cc
test_transient_for_CXXFLAGS = $(LIB_CFLAGS) -I../../src/lib -I../../src
test_transient_for_LDADD =  ../../src/tk/libpekwm_tk.a \
			    ../../src/lib/libpekwm_lib.a \
			   $(LIB_LIBS)

test_update_client_list_SOURCES = test_update_client_list.cc test_util.hh
test_update_client_list_CXXFLAGS = $(LIB_CFLAGS)
test_update_client_list_LDADD = $(LIB_LIBS)

EXTRA_DIST = CMakeLists.txt \
	     pekwm.autoproperties \
	     pekwm.config \
	     pekwm.config.report_all \
	     pekwm.config.vars \
	     pekwm.xresources \
	     pekwm_ctrl.xresources \
	     pekwm_panel.config.bottom \
	     pekwm_panel.config.pekwm_env \
	     pekwm_panel.config.systray \
	     pekwm_panel.config.top \
	     pekwm_sys.sh \
	     pekwm_themeset.config \
	     pekwm.plux \
	     pekwm_ctrl.plux \
	     pekwm_panel.plux \
	     pekwm_panel_systray.plux \
	     pekwm_screenshot.plux \
	     pekwm_theme.plux \
	     pekwm_themeset.plux \
	     test.pluxinc \
	     icons/term.png \
	     mock/netbsd/envstat_battery/envstat \
	     mock/netbsd/envstat_no_battery/envstat \
	     mock/openbsd/apm_no_battery/apm \
	     mock/openbsd/apm_battery/apm \
	     mock/openbsd/apm_ac/apm \
	     themes/with-dash/theme
