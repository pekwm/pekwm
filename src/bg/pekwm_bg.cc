//
// pekwm_bg.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "CfgParser.hh"
#include "Util.hh"
#include "X11.hh"

#include "../tk/CfgUtil.hh"
#include "../tk/ImageHandler.hh"
#include "../tk/TextureHandler.hh"
#include "TextureLinesAngle.hh"

extern "C" {
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
}

static bool _stop = false;
static std::string _config_script_path;
static ImageHandler* _image_handler = nullptr;
static TextureHandler* _texture_handler = nullptr;

namespace pekwm
{
	const std::string& configScriptPath()
	{
		return _config_script_path;
	}

	ImageHandler* imageHandler()
	{
		return _image_handler;
	}

	TextureHandler* textureHandler()
	{
		return _texture_handler;
	}
}

static void sighandler(int signal)
{
	if (signal == SIGINT) {
		_stop = true;
	}
}

static void init(Display* dpy, float scale)
{
	_image_handler = new ImageHandler(scale);
	_texture_handler = new TextureHandler(scale);
	_texture_handler->registerTexture("LINESANGLE", parseLinesAngle);
}

static void cleanup()
{
	delete _texture_handler;
	delete _image_handler;
}

static void usage(const char* name, int ret)
{
	std::cout << "usage: " << name << " [-CdDhls] texture" << std::endl;
	std::cout << "  -C --pekwm-config path (pekwm) Configuration file"
		  << std::endl;
	std::cout << "  -d --display dpy       Display" << std::endl;
	std::cout << "  -D --daemon            Run in the background"
		  << std::endl;
	std::cout << "  -h --help              Display this information"
		  << std::endl;
	std::cout << "  -l --load-dir path     Search path for images"
		  << std::endl;
	std::cout << "  -s --stop              Stop running pekwm_bg"
		  << std::endl;
	exit(ret);
}

static Pixmap setBackground(PTexture *tex)
{
	Pixmap pix = X11::createPixmap(X11::getWidth(), X11::getHeight());

	// render background per-head, might not suite all but leave it at
	// that for now.
	for (int i = 0; i < X11::getNumHeads(); i++) {
		Geometry head = X11::getHeadGeometry(i);
		tex->render(pix, head.x, head.y, head.width, head.height);
	}

	X11::setCardinal(X11::getRoot(), XROOTPMAP_ID, pix, XA_PIXMAP);
	X11::setCardinal(X11::getRoot(), XSETROOT_ID, pix, XA_PIXMAP);
	X11::setWindowBackgroundPixmap(X11::getRoot(), pix);
	X11::clearWindow(X11::getRoot());

	return pix;
}

static Pixmap loadAndSetBackground(const std::string& tex_str)
{
	PTexture *tex = pekwm::textureHandler()->getTexture(tex_str);
	if (! tex) {
		std::cerr << "Failed to load texture " << tex_str << std::endl;
		return None;
	}

	std::cout << "Setting background " << tex_str << std::endl;
	Pixmap pix = setBackground(tex);
	pekwm::textureHandler()->returnTexture(&tex);
	return pix;
}

static void modeBackground(const std::string& tex_str)
{
	Pixmap pix = loadAndSetBackground(tex_str);
	if (pix == None) {
		return;
	}

	// used for stop actions
	X11::setCardinal(X11::getRoot(), PEKWM_BG_PID, getpid());
	X11::selectXRandrInput();

	XEvent ev;
	ScreenChangeNotification scn;
	while (! _stop && X11::getNextEvent(ev)) {
		if (X11::getScreenChangeNotification(&ev, scn)) {
			X11::freePixmap(pix);
			pix = loadAndSetBackground(tex_str);
			if (pix == None) {
				break;
			}
		}
	}

	X11::freePixmap(pix);
}

void modeStop()
{
	Cardinal pid;
	if (! X11::getCardinal(X11::getRoot(), PEKWM_BG_PID, pid)) {
		return;
	}

	kill(pid, SIGINT);

	std::cout << "Sent pekwm_bg " << pid << " SIGINT" << std::endl;
}

int main(int argc, char* argv[])
{
	// Limit access, limit further after X11 connection is setup.
	pledge_x11_required("");

	const char* display = NULL;
	std::string pekwm_config_file = Util::getConfigDir() + "/config";
	bool do_daemon = false;
	bool stop = false;
	std::string load_dir("./");

	static struct option opts[] = {
		{const_cast<char*>("pekwm-config"), required_argument, nullptr,
		 'C'},
		{const_cast<char*>("display"), required_argument, nullptr,
		 'd'},
		{const_cast<char*>("daemon"), no_argument, nullptr, 'D'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("load-dir"), required_argument, nullptr,
		 'l'},
		{const_cast<char*>("stop"), no_argument, nullptr, 's'},
		{nullptr, 0, nullptr, 0}
	};

	int ch;
	while ((ch = getopt_long(argc, argv, "C:d:Dhl:s", opts, nullptr))
	       != -1) {
		switch (ch) {
		case 'C':
			pekwm_config_file = optarg;
			Util::expandFileName(pekwm_config_file);
			break;
		case 'd':
			display = optarg;
			break;
		case 'D':
			do_daemon = true;
			break;
		case 'h':
			usage(argv[0], 0);
			break;
		case 'l':
			load_dir = optarg;
			if (! load_dir.empty()
			    && load_dir[load_dir.size() - 1] != '/') {
				load_dir += '/';
			}
			Util::expandFileName(load_dir);
			break;
		case 's':
			stop = true;
			break;
		default:
			usage(argv[0], 1);
			break;
		}
	}

	std::string tex_str;
	for (int i = optind; i < argc; i++) {
		if (i > optind) {
			tex_str += " ";
		}
		tex_str += argv[i];
	}

	if (! stop && tex_str.empty()) {
		usage(argv[0], 1);
		return 1;
	}

	if (! X11::init(display, std::cerr, true)) {
		return 1;
	}

	// setup signal handler after connecting to the display
	signal(SIGINT, &sighandler);

	// X11 connection has been setup, limit access further
	pledge_x("stdio rpath proc", "");

	float scale;
	{
		CfgParser pekwm_cfg(CfgParserOpt(""));
		pekwm_cfg.parse(pekwm_config_file,
				CfgParserSource::SOURCE_FILE, true);
		CfgUtil::getScreenScale(pekwm_cfg.getEntryRoot(), scale);
	}

	init(X11::getDpy(), scale);

	_image_handler->path_push_back(load_dir);

	modeStop();

	// old process has been killed, limit access further
	pledge_x("stdio rpath", "");

	if (! stop) {
		if (do_daemon && daemon(0, 0) == -1) {
			std::cerr << "Failed to daemonize: " << strerror(errno)
				  << std::endl;
		}
		modeBackground(tex_str);
	}

	cleanup();
	X11::destruct();

	return 0;
}
