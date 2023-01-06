//
// pekwm_panel.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "CfgUtil.hh"
#include "Charset.hh"
#include "Compat.hh"
#include "Debug.hh"
#include "FontHandler.hh"
#include "ImageHandler.hh"
#include "Observable.hh"
#include "TextureHandler.hh"
#include "Util.hh"
#include "String.hh"
#include "X11App.hh"
#include "X11Util.hh"
#include "X11.hh"

#include "pekwm_panel.hh"
#include "ExternalCommandData.hh"
#include "PanelConfig.hh"
#include "PanelTheme.hh"
#include "PanelWidget.hh"
#include "VarData.hh"
#include "WidgetFactory.hh"
#include "WmState.hh"

extern "C" {
#include <assert.h>
#include <getopt.h>
#include <time.h>
}

/** pekwm configuration file. */
static std::string _pekwm_config_file;

/** empty string, used as default return value. */
static std::string _empty_string;

/** static pekwm resources, accessed via the pekwm namespace. */
static ObserverMapping* _observer_mapping = nullptr;
static FontHandler* _font_handler = nullptr;
static ImageHandler* _image_handler = nullptr;
static TextureHandler* _texture_handler = nullptr;

namespace pekwm
{
	ObserverMapping* observerMapping(void)
	{
		return _observer_mapping;
	}

	FontHandler* fontHandler()
	{
		return _font_handler;
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

static void
loadTheme(PanelTheme& theme, const std::string& pekwm_config_file)
{
	CfgParser cfg;
	cfg.parse(pekwm_config_file, CfgParserSource::SOURCE_FILE, true);

	std::string theme_dir, theme_variant, theme_path;
	bool font_default_x11;
	std::string font_charset_override;
	CfgUtil::getThemeDir(cfg.getEntryRoot(),
			     theme_dir, theme_variant, theme_path);
	CfgUtil::getFontSettings(cfg.getEntryRoot(),
				 font_default_x11,
				 font_charset_override);

	pekwm::fontHandler()->setDefaultFontX11(font_default_x11);
	pekwm::fontHandler()->setCharsetOverride(font_charset_override);
	theme.load(theme_dir, theme_path);

	std::string icon_path;
	CfgUtil::getIconDir(cfg.getEntryRoot(), icon_path);
	theme.setIconPath(icon_path, theme_dir + "/icons/");
}

typedef bool(*renderPredFun)(PanelWidget *w, void *opaque);

static bool renderPredExposeEv(PanelWidget *w, void *opaque)
{
	XExposeEvent *ev = reinterpret_cast<XExposeEvent*>(opaque);
	return (w->getX() >= ev->x) && (w->getRX() <= (ev->x + ev->width));
}

static bool renderPredDirty(PanelWidget *w, void*)
{
	return w->isDirty();
}

static bool renderPredAlways(PanelWidget*, void*)
{
	return true;
}

/**
 * Widgets in the panel are given a size when configured, can be given
 * in:
 *
 *   * pixels, number of pixels
 *   * percent, percent of the screen width the panel is on.
 *   * required, minimum required size.
 *   * *, all space not occupied by the other widgets. All * share the
 *     remaining space.
 *
 *       required           300px                         *
 * ----------------------------------------------------------------------------
 * | [WorkspaceNumber] |    [Text]   |              [ClientList]              |
 * ----------------------------------------------------------------------------
 *
 */
class PekwmPanel : public X11App,
		   public Observer {
public:
	PekwmPanel(const PanelConfig &cfg, PanelTheme &theme, XSizeHints *sh);
	virtual ~PekwmPanel(void);

	void configure(void);
	void setStrut(void);
	void place(void);
	void render(void);

	virtual void notify(Observable*, Observation *observation);
	virtual void refresh(bool timed_out);
	virtual void handleEvent(XEvent *ev);

private:

	virtual ActionEvent *handleButtonPress(XButtonEvent* ev)
	{
		X11::setLastEventTime(ev->time);
		PanelWidget *widget = findWidget(ev->x);
		if (widget != nullptr) {
			widget->click(ev->x - widget->getX(), ev->y - _gm.y);
		}
		return nullptr;
	}

	virtual ActionEvent *handleButtonRelease(XButtonEvent* ev)
	{
		X11::setLastEventTime(ev->time);
		return nullptr;
	}

	void handleExpose(XExposeEvent *ev)
	{
		renderPred(renderPredExposeEv, reinterpret_cast<void*>(ev));
	}

	void handlePropertyNotify(XPropertyEvent *ev)
	{
		X11::setLastEventTime(ev->time);
		if (_wm_state.handlePropertyNotify(ev)) {
			render();
		}
	}

	virtual void handleFd(int fd)
	{
		_ext_data.input(fd);
	}

	virtual void handleChildDone(pid_t pid, int)
	{
		_ext_data.done(pid, PekwmPanel::ppRemoveFd,
			       reinterpret_cast<void*>(this));
	}

	virtual void screenChanged(const ScreenChangeNotification&)
	{
		P_TRACE("screen geometry updated, resizing");
		place();
		resizeWidgets();
	}

	PanelWidget* findWidget(int x)
	{
		std::vector<PanelWidget*>::iterator it = _widgets.begin();
		for (; it != _widgets.end(); ++it) {
			if (x >= (*it)->getX() && x <= (*it)->getRX()) {
				return *it;
			}
		}
		return nullptr;
	}

	PanelWidget* findWidget(Window win)
	{
		std::vector<PanelWidget*>::iterator it = _widgets.begin();
		for (; it != _widgets.end(); ++it) {
			if (*(*it) == win) {
				return *it;
			}
		}
		return nullptr;
	}

	void addWidgets(void)
	{
		WidgetFactory factory(this, this, _theme,
				      _var_data, _wm_state);

		std::vector<WidgetConfig>::const_iterator it =
			_cfg.widgetsBegin();
		for (; it != _cfg.widgetsEnd(); ++it) {
			PanelWidget *widget = factory.construct(*it);
			if (widget == nullptr) {
				USER_WARN("failed to construct widget");
			} else {
				_widgets.push_back(widget);
			}
		}
	}

	void resizeWidgets(void);
	void renderPred(renderPredFun pred, void *opaque);
	void renderBackground(void);

	static void ppAddFd(int fd, void *opaque)
	{
		PekwmPanel *panel = reinterpret_cast<PekwmPanel*>(opaque);
		panel->addFd(fd);
	}

	static void ppRemoveFd(int fd, void *opaque)
	{
		PekwmPanel *panel = reinterpret_cast<PekwmPanel*>(opaque);
		panel->removeFd(fd);
	}

private:
	const PanelConfig& _cfg;
	PanelTheme& _theme;
	VarData _var_data;
	ExternalCommandData _ext_data;
	WmState _wm_state;
	std::vector<PanelWidget*> _widgets;
	Pixmap _pixmap;
};

PekwmPanel::PekwmPanel(const PanelConfig &cfg, PanelTheme &theme,
		       XSizeHints *sh)
	: X11App(Geometry(sh->x, sh->y,
			  static_cast<uint>(sh->width),
			  static_cast<uint>(sh->height)),
		 "", "panel", "pekwm_panel",
		 WINDOW_TYPE_DOCK, sh),
	  _cfg(cfg),
	  _theme(theme),
	  _ext_data(cfg, _var_data),
	  _wm_state(_var_data),
	  _pixmap(X11::createPixmap(sh->width, sh->height))
{
	X11::selectInput(_window,
			 ButtonPressMask|ButtonReleaseMask|
			 ExposureMask|
			 PropertyChangeMask|SubstructureNotifyMask);

	renderBackground();
	X11::setWindowBackgroundPixmap(_window, _pixmap);

	Atom state[] = {
		X11::getAtom(STATE_STICKY),
		X11::getAtom(STATE_SKIP_TASKBAR),
		X11::getAtom(STATE_SKIP_PAGER),
		X11::getAtom(STATE_ABOVE)
	};
	X11::setAtoms(_window, STATE, state, sizeof(state)/sizeof(state[0]));
	setStrut();

	// select root window for atom changes _before_ reading state
	// ensuring state is up-to-date at all times.
	X11::selectInput(X11::getRoot(), PropertyChangeMask);

	pekwm::observerMapping()->addObserver(&_wm_state, this);
}

PekwmPanel::~PekwmPanel(void)
{
	if (! _widgets.empty()) {
		pekwm::observerMapping()->removeObserver(&_var_data, this);
	}
	pekwm::observerMapping()->removeObserver(&_wm_state, this);
	std::vector<PanelWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		delete *it;
	}
	X11::freePixmap(_pixmap);
}

void
PekwmPanel::configure(void)
{
	if (! _widgets.empty()) {
		pekwm::observerMapping()->removeObserver(&_var_data, this);
	}
	addWidgets();
	if (! _widgets.empty()) {
		pekwm::observerMapping()->addObserver(&_var_data, this);
	}
	resizeWidgets();
}

void
PekwmPanel::setStrut(void)
{
	Cardinal strut[4] = {0};
	if (_cfg.getPlacement() == PANEL_TOP) {
		strut[2] = _theme.getHeight();
	} else {
		strut[3] = _theme.getHeight();
	}
	X11::setCardinals(_window, NET_WM_STRUT, strut, 4);
}

void
PekwmPanel::place(void)
{
	Geometry head = X11Util::getHeadGeometry(_cfg.getHead());

	int y;
	if (_cfg.getPlacement() == PANEL_TOP) {
		y = head.y;
	} else {
		y = head.y + head.height - _theme.getHeight();
	}
	moveResize(head.x, y, head.width, _theme.getHeight());
}

void
PekwmPanel::render(void)
{
	renderPred(renderPredDirty, nullptr);
}

void
PekwmPanel::notify(Observable*, Observation *observation)
{
	if (dynamic_cast<WmState::PEKWM_THEME_Changed*>(observation)) {
		P_DBG("reloading theme, _PEKWM_THEME changed");
		loadTheme(_theme, _pekwm_config_file);
		setStrut();
		place();
	}

	if (dynamic_cast<WmState::XROOTPMAP_ID_Changed*>(observation)
	    || dynamic_cast<WmState::PEKWM_THEME_Changed*>(observation)) {
		renderBackground();
		renderPred(renderPredAlways, nullptr);
	} else {
		if (dynamic_cast<RequiredSizeChanged*>(observation)) {
			P_TRACE("RequiredSizeChanged notification");
			resizeWidgets();
		}
		render();
	}
}

void
PekwmPanel::refresh(bool timed_out)
{
	_ext_data.refresh(ppAddFd, reinterpret_cast<void*>(this));
	if (timed_out) {
		renderPred(renderPredAlways, nullptr);
	}
}

void
PekwmPanel::handleEvent(XEvent* ev)
{
	PanelWidget *widget = findWidget(ev->xany.window);
	if (widget != nullptr) {
		if (widget->handleXEvent(ev)) {
			return;
		}
	}

	switch (ev->type) {
	case ButtonPress:
		P_TRACE("ButtonPress");
		handleButtonPress(&ev->xbutton);
		break;
	case ButtonRelease:
		P_TRACE("ButtonRelease");
		handleButtonRelease(&ev->xbutton);
		break;
	case ConfigureNotify:
		P_TRACE("ConfigureNotify");
		break;
	case DestroyNotify:
		P_TRACE("DestroyNotify");
		break;
	case EnterNotify:
		P_TRACE("EnterNotify");
		X11::setLastEventTime(ev->xcrossing.time);
		break;
	case Expose:
		P_TRACE("Expose");
		handleExpose(&ev->xexpose);
		break;
	case LeaveNotify:
		P_TRACE("LeaveNotify");
		X11::setLastEventTime(ev->xcrossing.time);
		break;
	case KeyPress:
	case KeyRelease:
		X11::setLastEventTime(ev->xkey.time);
		break;
	case MapNotify:
		P_TRACE("MapNotify");
		break;
	case MappingNotify:
		P_TRACE("MappingNotify");
		break;
	case MotionNotify:
		X11::setLastEventTime(ev->xkey.time);
		break;
	case ReparentNotify:
		P_TRACE("ReparentNotify");
		break;
	case UnmapNotify:
		P_TRACE("UnmapNotify");
		break;
	case PropertyNotify:
		P_TRACE("PropertyNotify");
		handlePropertyNotify(&ev->xproperty);
		break;
	case SelectionClear:
		X11::setLastEventTime(ev->xselectionclear.time);
		break;
	case SelectionRequest:
		X11::setLastEventTime(ev->xselectionrequest.time);
		break;
	case SelectionNotify:
		X11::setLastEventTime(ev->xselection.time);
		break;
	default:
		P_DBG("UNKNOWN EVENT " << ev->type);
		break;
	}
}

void
PekwmPanel::resizeWidgets(void)
{
	if (_widgets.empty()) {
		return;
	}

	PTexture *sep = _theme.getSep();
	uint num_rest = 0;
	uint width_left = _gm.width - sep->getWidth() * (_widgets.size() - 1);

	PTexture *handle = _theme.getHandle();
	if (handle) {
		width_left -= handle->getWidth() * 2;
	}

	std::vector<PanelWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		switch ((*it)->getSizeReq().getUnit()) {
		case WIDGET_UNIT_PIXELS:
			width_left -= (*it)->getSizeReq().getSize();
			(*it)->setWidth((*it)->getSizeReq().getSize());
			break;
		case WIDGET_UNIT_PERCENT: {
			uint width = static_cast<uint>(_gm.width
				  * (static_cast<float>(
					(*it)->getSizeReq().getSize()) / 100));
			width_left -= width;
			(*it)->setWidth(width);
			break;
		}
		case WIDGET_UNIT_REQUIRED:
			width_left -= (*it)->getRequiredSize();
			(*it)->setWidth((*it)->getRequiredSize());
			break;
		case WIDGET_UNIT_REST:
			num_rest++;
			break;
		case WIDGET_UNIT_TEXT_WIDTH: {
			PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
			uint width = font->getWidth(
					(*it)->getSizeReq().getText());
			width_left -= width;
			(*it)->setWidth(width);
			break;
		}
		}
	}

	uint x = handle ? handle->getWidth() : 0;
	uint rest = static_cast<uint>(width_left
				      / static_cast<float>(num_rest));
	for (it = _widgets.begin(); it != _widgets.end(); ++it) {
		if ((*it)->getSizeReq().getUnit() == WIDGET_UNIT_REST) {
			(*it)->setWidth(rest);
		}
		(*it)->move(x);
		x += (*it)->getWidth() + sep->getWidth();
	}
}

void
PekwmPanel::renderPred(renderPredFun pred, void *opaque)
{
	if (_widgets.empty()) {
		return;
	}

	PTexture *sep = _theme.getSep();
	PTexture *handle = _theme.getHandle();

	int x = handle ? handle->getWidth() : 0;
	PanelWidget *last_widget = _widgets.back();
	X11Render rend(_window);
	std::vector<PanelWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		bool do_render = pred(*it, opaque);
		if (do_render) {
			(*it)->render(rend);
		}
		x += (*it)->getWidth();

		if (do_render && *it != last_widget) {
			sep->render(rend, x, 0,
				    sep->getWidth(), sep->getHeight());
			x += sep->getWidth();
		}
	}
}

void
PekwmPanel::renderBackground(void)
{
	_theme.getBackground()->render(_pixmap, 0, 0, _gm.width, _gm.height,
				       _gm.x, _gm.y);
	PTexture *handle = _theme.getHandle();
	if (handle) {
		handle->render(_pixmap,
			       0, 0,
			       handle->getWidth(), handle->getHeight(),
			       0, 0); // root coordinates
		handle->render(_pixmap,
			       _gm.width - handle->getWidth(), 0,
			       handle->getWidth(), handle->getHeight(),
			       0, 0); // root coordinates
	}
}

static bool loadConfig(PanelConfig& cfg, const std::string& file)
{
	if (file.size() && cfg.load(file)) {
		return true;
	}

	std::string panel_config = Util::getConfigDir() + "/panel";
	if (cfg.load(panel_config)) {
		return true;
	}

	return cfg.load(SYSCONFDIR "/panel");
}

static void init(Display* dpy)
{
	_observer_mapping = new ObserverMapping();
	// options setup in loadTheme later on
	_font_handler = new FontHandler(false, "");
	_image_handler = new ImageHandler();
	_texture_handler = new TextureHandler();
}

static void cleanup()
{
	delete _texture_handler;
	delete _image_handler;
	delete _font_handler;
	delete _observer_mapping;
}

static void usage(const char* name, int ret)
{
	std::cout << "usage: " << name << " [-dh]" << std::endl;
	std::cout << " -c --config path    Configuration file" << std::endl;
	std::cout << " -d --display dpy    Display" << std::endl;
	std::cout << " -h --help           Display this information"
		  << std::endl;
	std::cout << " -f --log-file       Set log file." << std::endl;
	std::cout << " -l --log-level      Set log level." << std::endl;
	exit(ret);
}

int main(int argc, char *argv[])
{
	std::string config_file;
	const char* display = NULL;

	static struct option opts[] = {
		{const_cast<char*>("config"), required_argument, nullptr,
		 'c'},
		{const_cast<char*>("pekwm-config"), required_argument, nullptr,
		 'C'},
		{const_cast<char*>("display"), required_argument, nullptr,
		 'd'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("log-level"), required_argument, nullptr,
		 'l'},
		{const_cast<char*>("log-file"), required_argument, nullptr,
		 'f'},
		{nullptr, 0, nullptr, 0}
	};

	Charset::init();

	int ch;
	while ((ch = getopt_long(argc, argv, "c:C:d:f:hl:", opts, nullptr))
	       != -1) {
		switch (ch) {
		case 'c':
			config_file = optarg;
			break;
		case 'C':
			_pekwm_config_file = optarg;
			break;
		case 'd':
			display = optarg;
			break;
		case 'h':
			usage(argv[0], 0);
			break;
		case 'f':
			if (! Debug::setLogFile(optarg)) {
				std::cerr << "Failed to open log file "
					  << optarg << std::endl;
			}
			break;
		case 'l':
			Debug::setLevel(Debug::getLevel(optarg));
			break;
		default:
			usage(argv[0], 1);
			break;
		}
	}

	if (config_file.empty()) {
		config_file = Util::getConfigDir() + "/panel";
	}
	if (_pekwm_config_file.empty()) {
		_pekwm_config_file = Util::getConfigDir() + "/config";
	}
	Util::expandFileName(config_file);
	Util::expandFileName(_pekwm_config_file);

	Display *dpy = XOpenDisplay(display);
	if (! dpy) {
		std::string actual_display =
			display ? display : Util::getEnv("DISPLAY");
		std::cerr << "Can not open display!" << std::endl
			  << "Your DISPLAY variable currently is set to: "
			  << actual_display << std::endl;
		return 1;
	}

	X11::init(dpy, true);
	init(dpy);

	{
		// run in separate scope to get resources cleaned up before
		// X11 cleanup
		PanelConfig cfg;
		if (loadConfig(cfg, config_file)) {
			PanelTheme theme;
			loadTheme(theme, _pekwm_config_file);

			Geometry head =
				X11Util::getHeadGeometry(cfg.getHead());
			XSizeHints normal_hints = {0};
			normal_hints.flags = PPosition|PSize;
			normal_hints.x = head.x;
			normal_hints.width = head.width;
			normal_hints.height = theme.getHeight();

			if (cfg.getPlacement() == PANEL_TOP) {
				normal_hints.y = head.y;
			} else {
				normal_hints.y = head.y
					+ head.height - normal_hints.height;
			}

			PekwmPanel panel(cfg, theme, &normal_hints);
			panel.configure();
			panel.mapWindow();
			panel.render();
			panel.main(cfg.getRefreshIntervalS());
		} else {
			std::cerr << "failed to read panel configuration"
				  << std::endl;
		}
	}

	cleanup();
	X11::destruct();
	Charset::destruct();

	return 0;
}
