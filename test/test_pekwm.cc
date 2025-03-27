//
// test_pekwm.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"
#include "test.hh"

#include "Compat.hh"
#include "Debug.hh"

#include "test_Action.hh"
#include "test_ActionHandler.hh"
#include "test_AutoProperties.hh"
#include "test_Config.hh"
#include "test_ColorPalette.hh"
#include "test_FontHandler.hh"
#include "test_Frame.hh"
#include "test_InputDialog.hh"
#include "test_ManagerWindows.hh"
#include "test_Observable.hh"
#include "test_PFont.hh"
#ifdef PEKWM_HAVE_PANGO
#include "test_PFontPango.hh"
#endif // PEKWM_HAVE_PANGO
#include "test_PFontXmb.hh"
#include "test_PImage.hh"
#include "test_PMenu.hh"
#include "test_PSurface.hh"
#include "test_Theme.hh"
#include "test_WindowManager.hh"
#include "test_Workspaces.hh"
#include "test_X11.hh"

static int
main_tests(int argc, char *argv[])
{
	// Setup environment required for the tests
	Config cfg;
	HintWO hint_wo(None);
	Debug::setLogFile("/dev/null");
	X11::addHead(Head(0, 0, 800, 600));

	TestAutoProperties testAutoProperties;
	// Action
	TestAction testAction;
	TestActionConfig testActionConfig;
	TestActionHandler testActionHandler;

	// Config
	TestConfig testConfig;

	TestColorPalette testColorPalette;

	// Frame
	TestFrame testFrame;

	// InputDialog
	TestInputBuffer testInputBuffer;

	// ManagerWindows
	TestRootWO testRootWO(&hint_wo, &cfg);

	// Observable
	TestObserverMapping testObserverMapping;

	// FontHandler
	TestFontHandler testFontHandler;

	// PFont
	TestPFont testPFont;
#ifdef PEKWM_HAVE_PANGO
	TestPFontPango testPFontPango;
#endif // PEKWM_HAVE_PANGO
	TestPFontXmb testPFontXmb;

	// PMenu
	TestPMenu testPMenu;
	TestPSurface testPSurface;

	// Theme
	TestTheme testTheme;

	// WindowManager
	TestWindowManager testWindowManager;
	TestWorkspaces testWorkspaces;

	// x11
	TestX11 testX11;

	return TestSuite::main(argc, argv);
}

int
main(int argc, char *argv[])
{
	pekwm::initNoDisplay();
	int res = main_tests(argc, argv);
	pekwm::cleanupNoDisplay();

	return res;
}
