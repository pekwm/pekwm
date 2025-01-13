//
// pekwm_ctrl.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "Charset.hh"
#include "Daytime.hh"
#include "Debug.hh"
#include "Location.hh"
#include "RegexString.hh"
#include "Util.hh"
#include "X11.hh"

#include "../tk/X11Util.hh"

#include <algorithm>
#include <string>

extern "C" {
#include <getopt.h>
#include <time.h>
#include <unistd.h>
}

typedef bool(*send_message_fun)(Window, AtomName, int, const void*, size_t,
                                void *opaque);

enum CtrlAction {
	PEKWM_CTRL_ACTION_RUN,
	PEKWM_CTRL_ACTION_FOCUS,
	PEKWM_CTRL_ACTION_RESTACK,
	PEKWM_CTRL_ACTION_LIST,
	PEKWM_CTRL_ACTION_LIST_STACKING,
	PEKWM_CTRL_ACTION_LIST_CHILDREN,
	PEKWM_CTRL_ACTION_UTIL,
	PEKWM_CTRL_ACTION_XRM_GET,
	PEKWM_CTRL_ACTION_XRM_SET,
	PEKWM_CTRL_ACTION_NO
};

#ifndef UNITTEST

static const char *progname = nullptr;
static ObserverMapping* _observer_mapping = nullptr;

namespace pekwm
{
	ObserverMapping* observerMapping()
	{
		if (_observer_mapping == nullptr) {
			_observer_mapping = new ObserverMapping();
		}
		return _observer_mapping;
	}
}

static void usage(int ret)
{
	std::cout << "usage: " << progname << " [-acdhs] [command]"
		  << std::endl;
	std::cout << "  -a --action [run|focus|restack|list|list-stacking"
		  << "|list-children|util] Control action" << std::endl;
	std::cout << "  -c --client pattern Client pattern" << std::endl;
	std::cout << "  -C pattern          Other client pattern" << std::endl;
	std::cout << "  -d --display dpy    Display" << std::endl;
	std::cout << "  -h --help           Display this information"
		  << std::endl;
	std::cout << "  -g --xrm-get        Get string resource" << std::endl;
	std::cout << "  -s --xrm-set        Set string resource" << std::endl;
	std::cout << "  -w --window window  Client window" << std::endl;
	std::cout << "  -W window           Other client window" << std::endl;
	exit(ret);
}

static std::string formatTime(time_t ts)
{
	struct tm tm;
	localtime_r(&ts, &tm);
	char buf[32];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
	return buf;
}

static CtrlAction getAction(const std::string& name)
{
	if (name == "focus") {
		return PEKWM_CTRL_ACTION_FOCUS;
	} else if (name == "restack") {
		return PEKWM_CTRL_ACTION_RESTACK;
	} else if (name == "list") {
		return PEKWM_CTRL_ACTION_LIST;
	} else if (name == "list-stacking") {
		return PEKWM_CTRL_ACTION_LIST_STACKING;
	} else if (name == "list-children") {
		return PEKWM_CTRL_ACTION_LIST_CHILDREN;
	} else if (name == "run") {
		return PEKWM_CTRL_ACTION_RUN;
	} else if (name == "util") {
		return PEKWM_CTRL_ACTION_UTIL;
	} else {
		return PEKWM_CTRL_ACTION_NO;
	}
}

static std::string readClientName(Window win)
{
	std::string name;
	if (X11::getUtf8String(win, NET_WM_NAME, name)) {
		return name;
	}
	if (X11::getTextProperty(win, XA_WM_NAME, name)) {
		return name;
	}
	return "";
}

static std::string readPekwmTitle(Window win)
{
	std::string name;
	if (X11::getUtf8String(win, PEKWM_TITLE, name)) {
		return name;
	}
	return "";
}

static Window findClient(const RegexString& match)
{
	ulong actual;
	Window *windows;
	if (! X11::getProperty(X11::getRoot(), X11::getAtom(NET_CLIENT_LIST),
			       XA_WINDOW, 0,
			       reinterpret_cast<uchar**>(&windows), &actual)) {
		return None;
	}

	for (uint i = 0; i < actual; i++) {
		std::string name = readClientName(windows[i]);
		if (match == name) {
			return windows[i];
		}
	}

	X11::free(windows);

	return None;
}

static bool sendClientMessage(Window window, AtomName atom,
			      int format, const void* data, size_t size,
			      void *)
{
	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = X11::getAtom(atom);
	ev.xclient.window = window;
	ev.xclient.format = format;
	memset(ev.xclient.data.b, 0, sizeof(ev.xclient.data.b));
	if (size) {
		memcpy(ev.xclient.data.b, data,
		       std::min(size, sizeof(ev.xclient.data.b)));
	}

	return X11::sendEvent(X11::getRoot(), False,
			      SubstructureRedirectMask | SubstructureNotifyMask,
			      &ev);
}

static bool focusClient(Window win)
{
	return sendClientMessage(win, NET_ACTIVE_WINDOW, 32,
				 nullptr, 0, nullptr);
}

static bool restackWindow(int argc, char *argv[], Window win, Window sibling)
{
	if (win == None) {
		std::cout << "client window is required";
		return false;
	} else if (win == sibling) {
		std::cout << "client and other window must be different";
		return false;
	} else if (argc != 1) {
		std::cout << "must specify stacking detail";
		return false;
	}

	std::string detail_str(argv[0]);
	long detail = 0;
	if (detail_str == "above") {
		detail = Above;
	} else if (detail_str == "below") {
		detail = Below;
	} else if (detail_str == "topif") {
		detail = TopIf;
	} else if (detail_str == "bottomif") {
		detail = BottomIf;
	} else if (detail_str == "opposite") {
		detail = Opposite;
	} else {
		std::cerr << "unsupported detail " << detail_str << ", "
			  << "must be one of above, below, topif, bottomif "
			  << "or opposite";
		return false;
	}

	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = X11::getAtom(NET_RESTACK_WINDOW);
	ev.xclient.window = win;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 0;
	ev.xclient.data.l[1] = sibling;
	ev.xclient.data.l[2] = detail;
	X11::sendEvent(X11::getRoot(), False,
		       SubstructureRedirectMask | SubstructureNotifyMask, &ev);
	return true;
}

#endif // ! UNITTEST

static bool sendCommand(const std::string& cmd, Window win,
			send_message_fun send_message, void *opaque)
{
	XClientMessageEvent ev;
	char buf[sizeof(ev.data.b)] = {0};
	int chunk_size = sizeof(buf) - 1;

	const char *src = cmd.c_str();
	int left = cmd.size();
	buf[chunk_size] = static_cast<int>(cmd.size()) <= chunk_size ? 0 : 1;
	memcpy(buf, src, std::min(left, chunk_size));
	bool res = send_message(win, PEKWM_CMD, 8, buf, sizeof(buf), opaque);
	src += chunk_size;
	left -= chunk_size;
	while (res && left > 0) {
		memset(buf, 0, sizeof(buf));
		buf[chunk_size] = left <= chunk_size ? 3 : 2;
		memcpy(buf, src, std::min(left, chunk_size));
		src += chunk_size;
		left -= chunk_size;
		res = send_message(win, PEKWM_CMD, 8, buf, sizeof(buf), opaque);
	}

	return res;
}

#ifndef UNITTEST

static void printClient(Window win, const char *indent = "")
{
	std::string name = readClientName(win);
	std::string mb_name = Charset::toSystem(name);
	std::string pekwm_title = readPekwmTitle(win);
	std::string mb_pekwm_title = Charset::toSystem(pekwm_title);

	std::cout << indent << win << " " << mb_name;
	if (! mb_pekwm_title.empty()) {
		std::cout << " (" << mb_pekwm_title << ")";
	}
	std::cout << std::endl;
}

static bool listClients(bool stacking)
{
	ulong actual;
	Window *windows;
	Atom atom = X11::getAtom(stacking ? NET_CLIENT_LIST_STACKING
					  : NET_CLIENT_LIST);
	if (! X11::getProperty(X11::getRoot(), atom, XA_WINDOW, 0,
			       reinterpret_cast<uchar**>(&windows), &actual)) {
		return false;
	}

	for (uint i = 0; i < actual; i++) {
		printClient(windows[i]);
	}

	X11::free(windows);

	return true;
}

static bool listChildren()
{
	Window root, parent;
	std::vector<Window> wins;
	if (! X11::queryTree(X11::getRoot(), root, parent, wins)) {
		return false;
	}

	std::vector<Window>::iterator it(wins.begin());
	for (; it != wins.end(); ++it) {
		XWindowAttributes attr;
		X11::getWindowAttributes(*it, attr);
		if (attr.override_redirect || attr.map_state == IsUnmapped) {
			continue;
		}
		std::cout << *it << std::endl;

		std::vector<Window> c_wins;
		if (! X11::queryTree(*it, root, parent, c_wins)) {
			continue;
		}

		std::stringstream buf;
		std::vector<Window>::iterator c_it(c_wins.begin());
		for (; c_it != c_wins.end(); ++c_it) {
			std::string name = readClientName(*c_it);
			if (name.size() == 0) {
				if (buf.tellp()) {
					buf << ", ";
				}
				buf << *c_it;
			} else {
				printClient(*c_it, "  ");
			}
		}
		std::cout << "  * " << buf.str() << std::endl;
	}
	return true;
}

static void printRes(bool res)
{
	if (res) {
		std::cout << " OK" << std::endl;
	} else {
		std::cout << " ERROR" << std::endl;
	}
}

static bool actionRun(int argc, char** argv, Window client)
{
	if (client == None) {
		client = X11::getRoot();
	}

	std::string cmd;
	for (int i = 0; i < argc; i++) {
		if (i != 0) {
			cmd += " ";
		}
		cmd += argv[i];
	}

	if (cmd.empty()) {
		std::cerr << "empty command string" << std::endl;
		usage(1);
	}
	std::cout << "_PEKWM_CMD " << client << " " << cmd;
	bool res = sendCommand(cmd, client, sendClientMessage, nullptr);
	printRes(res);
	return res;
}

static int actionUtil(int argc, char* argv[])
{
	if (argc == 0) {
	        std::cerr << "no util command given" << std::endl;
		return 1;
	}

	std::string cmd(argv[0]);
	if (cmd == "timestamp") {
		time_t ts = time(nullptr);
		std::cout << std::to_string(ts) << std::endl;
		return 0;
	} else if (cmd == "location") {
		Location location(mkHttpClient());
		double latitude, longitude;
		if (! location.get(latitude, longitude)) {
			std::cerr << "failed to lookup location" << std::endl;
			return 1;
		}
		std::cout << "latitude: " << latitude << " longitude: "
			  << longitude << std::endl;
		return 0;
	} else if (cmd == "daytime") {
		Location location(mkHttpClient());
		double latitude, longitude;
		if (argc == 3) {
			try {
				latitude = std::stod(argv[1]);
				longitude = std::stod(argv[2]);
			} catch (std::invalid_argument &ex) {
				std::cerr << "invalid coordinates: "
					  << ex.what() << std::endl;
				return 1;
			}
		} else if (! location.get(latitude, longitude)) {
			std::cerr << "failed to lookup location" << std::endl;
			return 1;
		}
		Daytime daytime(time(NULL), latitude, longitude);
		std::cout << "sun rise: " << formatTime(daytime.getSunRise())
			  << " sun set: " << formatTime(daytime.getSunSet())
			  << std::endl;
		return 0;
	} else {
		std::cerr << "unknown util command: " << argv[0] << std::endl;
		return 1;
	}
}

static bool actionXrmGet(const std::string& key)
{
	if (key.empty()) {
		std::cerr << "no resource given" << std::endl;
		usage(1);
	}

	std::string val;
	if (X11::getXrmString(key, val)) {
		std::cout << val << std::endl;
		return true;
	}
	return false;
}

static bool actionXrmSet(int argc, char** argv)
{
	for (int i = 0; i < argc; i++) {
		std::vector<std::string> key_value;
		if (Util::splitString(argv[i], key_value, "=", 2) == 2) {
			X11::setXrmString(key_value[0], key_value[1]);
		}
	}

	X11Util::updateXrmResources();
	return true;

}

static void parseWinId(Window &win, RegexString &re, const char *arg,
		       char c, char w)
{
	if (re.is_match_ok()) {
		std::cerr << "-" << c << " and -" << w << " are mutually "
			  << "exclusive" << std::endl;
		usage(1);
	}
	try {
		win = std::stoi(optarg);
	} catch (std::invalid_argument&) {
		std::cerr << "invalid client id " << optarg << " given, "
			  << "expect a number" << std::endl;
	}
}

static bool findClientRe(Window &win, RegexString &re)
{
	if (!re.is_match_ok()) {
		return true;
	}
	win = findClient(re);
	if (win == None) {
		std::cerr << "no client match "
			  << Charset::toSystem(re.getPattern()) << std::endl;
		return false;
	}
	return true;
}

int main(int argc, char* argv[])
{
	pledge_x11_required("");

	progname = argv[0];
	const char* display = NULL;

	static struct option opts[] = {
		{const_cast<char*>("action"), required_argument, nullptr, 'a'},
		{const_cast<char*>("client"), required_argument, nullptr, 'c'},
		{const_cast<char*>("display"), required_argument, nullptr, 'd'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("xrm-get"), required_argument, nullptr, 'g'},
		{const_cast<char*>("xrm-set"), no_argument, nullptr, 's'},
		{const_cast<char*>("window"), required_argument, nullptr, 'w'},
		{nullptr, 0, nullptr, 0}
	};

	Charset::init();

	int ch;
	CtrlAction action = PEKWM_CTRL_ACTION_RUN;
	std::string val;
	Window client = None;
	Window sibling = None;
	RegexString client_re;
	RegexString sibling_re;
	while ((ch = getopt_long(argc, argv, "a:c:C:d:g:hsw:W:", opts, nullptr))
	       != -1) {
		switch (ch) {
		case 'a':
			action = getAction(optarg);
			if (action == PEKWM_CTRL_ACTION_NO) {
				usage(1);
			}
			break;
		case 'c':
			if (client != None) {
				std::cerr << "-c and -w are mutually "
					  << "exclusive" << std::endl;
				usage(1);
			}
			client_re.parse_match(optarg);
			break;
		case 'C':
			if (sibling != None) {
				std::cerr << "-C and -W are mutually "
					  << "exclusive" << std::endl;
				usage(1);
			}
			sibling_re.parse_match(optarg);
			break;
		case 'd':
			display = optarg;
			break;
		case 'g':
			action = PEKWM_CTRL_ACTION_XRM_GET;
			val = optarg;
			break;
		case 's':
			action = PEKWM_CTRL_ACTION_XRM_SET;
			break;
		case 'h':
			usage(0);
			break;
		case 'w':
			parseWinId(client, client_re, optarg, 'c', 'w');
			break;
		case 'W':
			parseWinId(sibling, sibling_re, optarg, 'C', 'W');
			break;

		default:
			usage(1);
			break;
		}
	}

	if (action == PEKWM_CTRL_ACTION_UTIL) {
		return actionUtil(argc - optind, argv + optind);
	}

	if (! X11::init(display, std::cerr)) {
		return 1;
	}

	// X11 connection has been setup, limit access further
	pledge_x("stdio", "");

	if (!findClientRe(client, client_re)
	    || !findClientRe(sibling, sibling_re)) {
		return 1;
	}

	bool res;
	switch (action) {
	case PEKWM_CTRL_ACTION_RUN:
		res = actionRun(argc - optind, argv + optind, client);
		break;
	case PEKWM_CTRL_ACTION_FOCUS:
		std::cout << "_NET_ACTIVE_WINDOW " << client;
		res = focusClient(client);
		printRes(res);
		break;
	case PEKWM_CTRL_ACTION_RESTACK:
		std::cout << "_NET_RESTACK_WINDOW " << client << " "
			  << sibling;
		res = restackWindow(argc - optind, argv + optind, client,
				    sibling);
		printRes(res);
		break;
	case PEKWM_CTRL_ACTION_LIST:
		std::cout << "_NET_CLIENT_LIST" << std::endl;
		res = listClients(false);
		printRes(res);
		break;
	case PEKWM_CTRL_ACTION_LIST_STACKING:
		std::cout << "_NET_CLIENT_LIST_STACKING" << std::endl;
		res = listClients(true);
		printRes(res);
		break;
	case PEKWM_CTRL_ACTION_LIST_CHILDREN:
		std::cout << "XQueryTree" << std::endl;
		res = listChildren();
		printRes(res);
		break;
	case PEKWM_CTRL_ACTION_XRM_GET: {
		res = actionXrmGet(val);
		break;
	}
	case PEKWM_CTRL_ACTION_XRM_SET:
		res = actionXrmSet(argc - optind, argv + optind);
		break;
	case PEKWM_CTRL_ACTION_NO:
	case PEKWM_CTRL_ACTION_UTIL:
		res = false;
		break;
	}

	X11::destruct();
	Charset::destruct();
	delete _observer_mapping;

	return res ? 0 : 1;
}

#endif // ! UNITTEST
