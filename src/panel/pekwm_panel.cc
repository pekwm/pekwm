//
// pekwm_panel.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "Compat.hh"
#include "Cond.hh"
#include "Debug.hh"
#include "Os.hh"
#include "Observable.hh"
#include "Util.hh"
#include "String.hh"
#include "X11.hh"
#include "../pekwm_env.hh"

#include "pekwm_panel.hh"
#include "ExternalCommandData.hh"
#include "PanelConfig.hh"
#include "PanelTheme.hh"
#include "PanelWidget.hh"
#include "VarData.hh"
#include "WidgetFactory.hh"
#include "WmState.hh"

#include "../tk/CfgUtil.hh"
#include "../tk/FontHandler.hh"
#include "../tk/ImageHandler.hh"
#include "../tk/TextureHandler.hh"
#include "../tk/ThemeUtil.hh"
#include "../tk/X11App.hh"
#include "../tk/X11Util.hh"

extern "C" {
#include <assert.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
}

/** pekwm configuration file. */
static std::string _pekwm_config_file;

/** empty string, used as default return value. */
static std::string _empty_string;

/** static pekwm resources, accessed via the pekwm namespace. */
static std::string _config_script_path;
static ObserverMapping* _observer_mapping = nullptr;
static FontHandler* _font_handler = nullptr;
static ImageHandler* _image_handler = nullptr;
static TextureHandler* _texture_handler = nullptr;

namespace pekwm
{
	const std::string& configScriptPath()
	{
		return _config_script_path;
	}

	ObserverMapping* observerMapping()
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
loadTheme(PanelTheme& theme, CfgParser& cfg)
{
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
	ThemeUtil::loadRequire(cfg, theme_dir, theme_path);
	theme.load(theme_dir, theme_path);

	std::string icon_path;
	CfgUtil::getIconDir(cfg.getEntryRoot(), icon_path);
	theme.setIconPath(icon_path, theme_dir + "/icons/");
}

static void
loadTheme(PanelTheme& theme, const std::string& pekwm_config_file)
{
	CfgParser cfg(CfgParserOpt(""));
	cfg.parse(pekwm_config_file, CfgParserSource::SOURCE_FILE, true);
	loadTheme(theme, cfg);
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
	void render();

	virtual void notify(Observable*, Observation *observation);
	virtual void refresh(bool timed_out);
	virtual void handleEvent(XEvent *ev);

private:
	virtual ActionEvent *handleButtonPress(XButtonEvent* ev)
	{
		X11::setLastEventTime(ev->time);
		PanelWidget *widget = findWidget(ev->x);
		if (widget != nullptr) {
			widget->click(ev->button,
				      ev->x - widget->getX(), ev->y - _gm.y);
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

	virtual void themeChanged(const std::string& name,
				  const std::string& variant, float scale)
	{
		// scale changed, update before re-loadint the theme
		bool scale_changed =
			scale != pekwm::textureHandler()->getScale();
		if (scale_changed) {
			pekwm::fontHandler()->setScale(scale);
			pekwm::imageHandler()->setScale(scale);
			pekwm::textureHandler()->setScale(scale);
		}

		loadTheme(_theme, _pekwm_config_file);
		setStrut();
		place();
		// resize widgets after loading, separator size, handles and
		// scale can alter available amount of space.
		resizeWidgets(scale_changed);

		// re-render
		renderBackground();
		renderPred(renderPredAlways, nullptr);
	}

	PanelWidget* findWidget(int x)
	{
		std::vector<PanelWidget*>::iterator it = _widgets.begin();
		for (; it != _widgets.end(); ++it) {
			if ((*it)->isVisible()
			    && x >= (*it)->getX()
			    && x <= (*it)->getRX()) {
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

	void addWidgets();
	void resizeWidgets(bool scale_changed=false);
	void renderPred(renderPredFun pred, void *opaque);
	void renderBackground();
	int renderHandle(Drawable drawable)
	{
		PTexture *handle = _theme.getHandle();
		if (! handle) {
			return 0;
		}
		handle->render(drawable,
			       0, 0,
			       handle->getWidth(), handle->getHeight(), 0, 0);
		handle->render(drawable,
			       _gm.width - handle->getWidth(), 0,
			       handle->getWidth(), handle->getHeight(), 0, 0);
		return handle->getWidth();
	}

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
	Os* _os;
	const PanelConfig& _cfg;
	PanelTheme& _theme;
	VarData _var_data;
	ExternalCommandData _ext_data;
	WmState _wm_state;
	std::vector<PanelWidget*> _widgets;
	uint _widgets_visible;

	PPixmapSurface _background;
};

PekwmPanel::PekwmPanel(const PanelConfig &cfg, PanelTheme &theme,
		       XSizeHints *sh)
	: X11App(Geometry(sh->x, sh->y,
			  static_cast<uint>(sh->width),
			  static_cast<uint>(sh->height)),
		 X_VALUE|Y_VALUE|WIDTH_VALUE|HEIGHT_VALUE,
		 "", "panel", "pekwm_panel",
		 WINDOW_TYPE_DOCK, sh, true),
	  _os(mkOs()),
	  _cfg(cfg),
	  _theme(theme),
	  _ext_data(cfg, _var_data),
	  _wm_state(_var_data),
	  _widgets_visible(0),
	  _background(sh->width, sh->height)
{
	X11::selectInput(_window,
			 ButtonPressMask|ButtonReleaseMask|
			 ExposureMask|
			 PropertyChangeMask|SubstructureNotifyMask);

	renderBackground();
	setBackground(_background.getDrawable());

	Atom state[] = {
		X11::getAtom(STATE_STICKY),
		X11::getAtom(STATE_SKIP_TASKBAR),
		X11::getAtom(STATE_SKIP_PAGER),
		X11::getAtom(STATE_ABOVE)
	};
	X11::setAtoms(_window, STATE, state, sizeof(state)/sizeof(state[0]));
	setStrut();

	pekwm::observerMapping()->addObserver(&_wm_state, this, 100);
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

	delete _os;
}

void
PekwmPanel::configure(void)
{
	if (! _widgets.empty()) {
		pekwm::observerMapping()->removeObserver(&_var_data, this);
	}
	addWidgets();
	if (! _widgets.empty()) {
		pekwm::observerMapping()->addObserver(&_var_data, this, 100);
	}
	resizeWidgets();

	_wm_state.read();
}

void
PekwmPanel::setStrut(void)
{
	Cardinal strut[STRUT_SIZE] = {0};
	if (_cfg.getPlacement() == PANEL_TOP) {
		strut[STRUT_TOP] = _theme.getHeight();
	} else {
		strut[STRUT_BOTTOM] = _theme.getHeight();
	}
	X11::setCardinals(_window, NET_WM_STRUT, strut, PEKWM_ARRLEN(strut));
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
PekwmPanel::notify(Observable* o, Observation *observation)
{
	if (dynamic_cast<WmState::XROOTPMAP_ID_Changed*>(observation)) {
		renderBackground();
		renderPred(renderPredAlways, nullptr);
	} else if (dynamic_cast<RequiredSizeChanged*>(observation)) {
		P_LOG("RequiredSizeChanged notification");
		resizeWidgets();
		renderBackground();
		renderPred(renderPredAlways, nullptr);
	} else {
		renderPred(renderPredDirty, nullptr);
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
		P_TRACE("Expose " << ev->xexpose.x << "x"
			<< ev->xexpose.y << "x" << ev->xexpose.width << "x"
			<< ev->xexpose.height);
		handleExpose(&ev->xexpose);
		break;
	case NoExpose:
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
PekwmPanel::addWidgets(void)
{
	WidgetFactory factory(_os, this, this, _theme, _var_data, _wm_state);

	std::vector<WidgetConfig>::const_iterator it = _cfg.widgetsBegin();
	for (; it != _cfg.widgetsEnd(); ++it) {
		PanelWidget *widget = factory.construct(*it);
		if (widget == nullptr) {
			USER_WARN("failed to construct widget");
			continue;
		}

		_widgets.push_back(widget);
	}
}

void
PekwmPanel::resizeWidgets(bool scale_changed)
{
	if (_widgets.empty()) {
		return;
	}

	PTexture *sep = _theme.getSep();
	uint num_rest = 0;
	uint width_left = _gm.width;
	PTexture *handle = _theme.getHandle();
	if (handle) {
		width_left -= handle->getWidth() * 2;
	}

	uint width;
	PFont *font;
	float scale = pekwm::textureHandler()->getScale();

	_widgets_visible = 0;
	std::vector<PanelWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		if (! Cond::eval((*it)->getIf())) {
			continue;
		}
		if (scale_changed) {
			(*it)->scaleChanged();
		}

		switch ((*it)->getSizeReq().getUnit()) {
		case WIDGET_UNIT_PIXELS:
			width = ThemeUtil::scaledPixelValue(
				scale, (*it)->getSizeReq().getSize());
			width_left -= width;
			(*it)->setWidth(width);
			break;
		case WIDGET_UNIT_PERCENT:
			width = static_cast<uint>(_gm.width
				  * (static_cast<float>(
					(*it)->getSizeReq().getSize()) / 100));
			width_left -= width;
			(*it)->setWidth(width);
			break;
		case WIDGET_UNIT_REQUIRED:
			width_left -= (*it)->getRequiredSize();
			(*it)->setWidth((*it)->getRequiredSize());
			break;
		case WIDGET_UNIT_REST:
			num_rest++;
			break;
		case WIDGET_UNIT_TEXT_WIDTH:
			font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
			width = font->getWidth((*it)->getSizeReq().getText());
			width_left -= width;
			(*it)->setWidth(width);
			break;
		}

		if ((*it)->isVisible()) {
			_widgets_visible++;
		}
	}

	// remove size for separators before calculating rest widget sizes,
	// this is done after the main calculation to get the correct number
	// of visible widgets
	if (_widgets_visible != 0) {
		width_left -=  sep->getWidth() * (_widgets_visible - 1);
	}
	uint x = handle ? handle->getWidth() : 0;
	uint rest = static_cast<uint>(width_left
				      / static_cast<float>(num_rest));
	for (it = _widgets.begin(); it != _widgets.end(); ++it) {
		if ((*it)->getSizeReq().getUnit() == WIDGET_UNIT_REST) {
			(*it)->setWidth(rest);
		}
		if (! (*it)->isVisible()) {
			continue;
		}
		(*it)->move(x);
		x += (*it)->getWidth() + sep->getWidth();
	}
}

void
PekwmPanel::renderPred(renderPredFun pred, void *opaque)
{
	if (_widgets_visible == 0) {
		P_TRACE("no visible widgets to render");
		return;
	}

	int x = renderHandle(getRenderDrawable());
	PanelWidget *last_widget = _widgets.back();
	X11Render rend(getRenderDrawable(), getRenderBackground());
	RenderSurface surface(rend, _gm);
	std::vector<PanelWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		if (! (*it)->isVisible()) {
			continue;
		}

		bool do_render = pred(*it, opaque);
		if (do_render) {
			(*it)->render(rend, &surface);
		}
		x += (*it)->getWidth();

		if (*it != last_widget) {
			PTexture *sep = _theme.getSep();
			if (do_render) {
				sep->render(rend, x, 0,
					    sep->getWidth(), sep->getHeight());
			}
			x += sep->getWidth();
		}
	}

	swapBuffer();
}

void
PekwmPanel::renderBackground()
{
	if (_background.resize(_gm.width, _gm.height)) {
		setBackground(_background.getDrawable());
	}
	_theme.getBackground()->render(_background.getDrawable(), 0, 0,
				       _gm.width, _gm.height, _gm.x, _gm.y);
	renderHandle(_background.getDrawable());
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

static void init(Display* dpy, float scale)
{
	_observer_mapping = new ObserverMapping();
	// options setup in loadTheme later on
	_font_handler = new FontHandler(scale, false, "");
	_image_handler = new ImageHandler(scale);
	_texture_handler = new TextureHandler(scale);
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
	std::cout << " -C --pekwm-config path pekwm Configuration file"
		  << std::endl;
	std::cout << " -d --display dpy    Display" << std::endl;
	std::cout << " -h --help           Display this information"
		  << std::endl;
	std::cout << " -f --log-file       Set log file." << std::endl;
	std::cout << " -l --log-level      Set log level." << std::endl;
	exit(ret);
}

int main(int argc, char *argv[])
{
	pledge_x11_required("");

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
	initEnv(false);

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
	initEnvConfig(Util::getDir(_pekwm_config_file), _pekwm_config_file);

	if (! X11::init(display, std::cerr)) {
		return 1;
	}

	Destruct<CfgParser> pekwm_cfg(new CfgParser(CfgParserOpt("")));
	(*pekwm_cfg)->parse(_pekwm_config_file, CfgParserSource::SOURCE_FILE,
			    true);
	float scale;
	CfgUtil::getScreenScale((*pekwm_cfg)->getEntryRoot(), scale);

	init(X11::getDpy(), scale);

	P_TRACE("pekwm_panel PID " << getpid());
	{
		PanelTheme theme;
		CfgUtil::getScriptsDir((*pekwm_cfg)->getEntryRoot(),
				       _config_script_path);
		loadTheme(theme, *(*pekwm_cfg));

		// free up resources, pekwm_cfg will not be used after this.
		pekwm_cfg.destruct();

		// run in separate scope to get resources cleaned up before
		// X11 cleanup
		PanelConfig cfg;
		if (loadConfig(cfg, config_file)) {

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
