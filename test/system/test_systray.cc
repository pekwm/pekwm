/**
 * Client used for testing systray functionality of pekwm_panel.
 */

#include <cstdio>
#include <cstring>
#include <iostream>

extern "C" {
#include <assert.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
}

enum InputType {
	INPUT_NO = 0,
	INPUT_STDIN = 1,
	INPUT_XEVENT = 2
};

static const long XEMBED_VERSION = 0;
static const long XEMBED_FLAG_MAPPED = 1 << 0;

enum SystrayOpcode {
	SYSTEM_TRAY_REQUEST_DOCK = 0,
	SYSTEM_TRAY_BEGIN_MESSAGE = 1,
	SYSTEM_TRAY_CANCEL_MESSAGE = 2
};

static void
send_message(Display* dpy, Window w, long message,
	     long data1, long data2, long data3)
{
    XEvent ev = {0};

    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type =
	    XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = data1;
    ev.xclient.data.l[3] = data2;
    ev.xclient.data.l[4] = data3;

    XSendEvent(dpy, w, False, NoEventMask, &ev);
    XSync(dpy, False);
}

void
set_xembed_info(Display* dpy, Window w, long xembed_flags)
{
	std::cout << "set _XEMBED_INFO " << xembed_flags << std::endl;
	Atom xembed_info = XInternAtom(dpy, "_XEMBED_INFO", False);
	long data[2] = { XEMBED_VERSION, xembed_flags };
	XChangeProperty(dpy, w, xembed_info, xembed_info,
			32, PropModeReplace,
			reinterpret_cast<unsigned char*>(data), 2);
}

static Window
wait_for_owner(Display* dpy, Atom owner_atom)
{
	Window owner = XGetSelectionOwner(dpy, owner_atom);
	while (owner == None) {
		sleep(1);
		owner = XGetSelectionOwner(dpy, owner_atom);
	}
	return owner;
}

static enum InputType
wait_for_input(Display* dpy, XEvent* ev)
{
	if (XPending(dpy)) {
		XNextEvent(dpy, ev);
		return INPUT_XEVENT;
	}

	int dpy_fd = ConnectionNumber(dpy);
	int stdin_fd = fileno(stdin);

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(dpy_fd, &rfds);
	FD_SET(stdin_fd, &rfds);

	int ret = select(std::max(dpy_fd, stdin_fd) + 1, &rfds, 0, 0, 0);
	if (ret > 0) {
		if (FD_ISSET(dpy_fd, &rfds)) {
			XNextEvent(dpy, ev);
			return INPUT_XEVENT;
		}
		if (FD_ISSET(stdin_fd, &rfds)) {
			return INPUT_STDIN;
		}
	}

	return INPUT_NO;
}

static void
handle_xevent(Display* dpy, XEvent* ev)
{
	switch (ev->type) {
	case ButtonPress:
		std::cout << "ButtonPress" << std::endl;
		break;
	case ClientMessage:
		std::cout << "ClientMessage" << std::endl;
		break;
	default:
		std::cout << "Other Event " << ev->type << std::endl;
	}
}

static void
handle_stdin(Display* dpy, Window w)
{
	std::string cmd;
	std::cin >> cmd;
	if (cmd == "map") {
		set_xembed_info(dpy, w, XEMBED_FLAG_MAPPED);
	} else if (cmd == "unmap") {
		set_xembed_info(dpy, w, 0);
	} else {
		std::cout << "unknown command: " << cmd;
	}
}

int
main(int argc, char *argv[])
{
	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		std::cerr << "ERROR: unable to open display" << std::endl;
		return 1;
	}

	// parse options
	long xembed_flags = XEMBED_FLAG_MAPPED;
	bool set_xembed = true;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i],  "unmapped") == 0) {
			xembed_flags = 0;
		} else if (strcmp(argv[i], "skip-xembed") == 0) {
			set_xembed = false;
		}
	}

	int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	// Find system tray owner
	Atom owner_atom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	assert(owner_atom != None);
	Window owner = XGetSelectionOwner(dpy, owner_atom);
	if (owner == None) {
		std::cout << "no session owner" << std::endl;
		owner = wait_for_owner(dpy, owner_atom);
	}

	Window w = XCreateSimpleWindow(dpy, root, -32, -32, 32, 32, 0, 0, 0);
	XSelectInput(dpy, w, ButtonPressMask);

	// _NET_WM_NAME
	Atom utf8_string = XInternAtom(dpy, "UTF8_STRING", False);
	Atom net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
	XChangeProperty(dpy, w, net_wm_name, utf8_string, 8, PropModeReplace,
			reinterpret_cast<const unsigned char*>("test_systray"),
			12);

	// Setup _XEMBED_INFO for client
	if (set_xembed) {
		set_xembed_info(dpy, w, xembed_flags);
	}

	std::cout << "send dock request for " << w << " to " << owner
		  << std::endl;
	send_message(dpy, owner, SYSTEM_TRAY_REQUEST_DOCK, w, 0, 0);

	std::cout << "waiting for event" << std::endl;
	enum InputType type;
	XEvent ev;
	while ((type = wait_for_input(dpy, &ev)) != 0) {
		if (type == INPUT_XEVENT) {
			handle_xevent(dpy, &ev);
		} else if (type == INPUT_STDIN) {
			handle_stdin(dpy, w);
		}
	}

	XCloseDisplay(dpy);

	return 0;
}
