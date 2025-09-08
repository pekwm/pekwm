SUBDIRS = system

TEST_CXXFLAGS = -I../src/lib -I../src $(LIB_CXXFLAGS) $(TK_CFLAGS)
TEST_LDADD = ../src/tk/libpekwm_tk.a ../src/lib/libpekwm_lib.a \
	     $(LIB_LIBS) $(TK_LIBS)

bin_PROGRAMS = test_pekwm \
	       test_pekwm_ctrl \
	       test_pekwm_panel \
	       test_pekwm_panel_sysinfo \
	       test_util \
	       test_pekwm_sys

test_pekwm_SOURCES = test_pekwm.cc \
		     test_Action.hh \
		     test_ActionHandler.hh \
		     test_AutoProperties.hh \
		     test_Config.hh \
		     test_ColorPalette.hh \
		     test_FontHandler.hh \
		     test_Frame.hh \
		     test_InputDialog.hh \
		     test_ManagerWindows.hh \
		     test_Observable.hh \
		     test_PFont.hh \
		     test_PFontPango.hh \
		     test_PFontXmb.hh \
		     test_PImage.hh \
		     test_PMenu.hh \
		     test_PSurface.hh \
		     test_Theme.hh \
		     test_WindowManager.hh \
		     test_Workspaces.hh \
		     test_X11.hh \
		     ../src/pekwm_env.cc
test_pekwm_CXXFLAGS = $(TEST_CXXFLAGS)
test_pekwm_LDADD = ../src/wm/libpekwm_wm.a $(TEST_LDADD)

test_pekwm_ctrl_SOURCES = test_pekwm_ctrl.cc
test_pekwm_ctrl_CXXFLAGS = $(TEST_CXXFLAGS)
test_pekwm_ctrl_LDADD = $(TEST_LDADD)

test_pekwm_panel_SOURCES = test_pekwm_panel.cc \
			   ../src/pekwm_env.cc \
			   ../src/panel/ClientInfo.cc \
			   ../src/panel/ExternalCommandData.cc \
			   ../src/panel/TextFormatter.cc \
			   ../src/panel/PanelConfig.cc \
			   ../src/panel/VarData.cc \
			   ../src/panel/WmState.cc \
			   test_ExternalCommandData.hh \
			   test_TextFormatter.hh
test_pekwm_panel_CXXFLAGS = $(TEST_CXXFLAGS)
test_pekwm_panel_LDADD = ../src/wm/libpekwm_wm.a $(TEST_LDADD) $(WM_LIBS)

test_pekwm_panel_sysinfo_SOURCES = test_pekwm_panel_sysinfo.cc
test_pekwm_panel_sysinfo_CXXFLAGS = $(TEST_CXXFLAGS)
test_pekwm_panel_sysinfo_LDADD = $(TEST_LDADD)

test_util_SOURCES = test_util.cc \
		    test_Calendar.hh \
		    test_CfgParser.hh \
		    test_Charset.hh \
		    test_Cond.hh \
		    test_Daytime.hh \
		    test_Geometry.hh \
		    test_Json.hh \
		    test_Location.hh \
		    test_Mem.hh \
		    test_Md5.hh \
		    test_Os.hh \
		    test_RegexString.hh \
		    test_String.hh \
		    test_Timeouts.hh \
		    test_Tokenizer.hh \
		    test_Util.hh

test_util_CXXFLAGS = $(TEST_CXXFLAGS)
test_util_LDADD = $(TEST_LDADD)

test_pekwm_sys_SOURCES = test_pekwm_sys.cc \
			 ../src/pekwm_env.cc \
			 ../src/sys/SysConfig.cc \
			 ../src/sys/SysMonitorChange.cc \
			 ../src/sys/SysMonitorConfig.cc \
			 ../src/sys/SysResources.cc \
			 ../src/sys/XSettings.cc \
			 test_PekwmSys.hh \
			 test_SysConfig.hh \
			 test_SysMonitorConfig.hh \
			 test_SysResources.hh \
			 test_XSettings.hh

test_pekwm_sys_CXXFLAGS = $(TEST_CXXFLAGS) $(libcurl_CFLAGS) $(libcurl_LIBS)
test_pekwm_sys_LDADD = $(TEST_LDADD) $(libcurl_LIBS) $(libudev_LIBS)

EXTRA_DIST = CMakeLists.txt \
	     data/cfg_parser_command.sh \
	     data/cfg_parser_include.cfg \
	     data/config.pekwm_sys \
	     data/config.pekwm_sys.invalid_lat \
	     data/config.pekwm_sys.invalid_long \
	     data/linux_proc_meminfo \
	     test.hh \
	     test_Mock.hh
