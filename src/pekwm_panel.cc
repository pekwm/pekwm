//
// pekwm_panel.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "Charset.hh"
#include "Compat.hh"
#include "Debug.hh"
#include "FontHandler.hh"
#include "ImageHandler.hh"
#include "Observable.hh"
#include "PImageIcon.hh"
#include "TextureHandler.hh"
#include "Util.hh"
#include "X11App.hh"
#include "X11Util.hh"
#include "X11.hh"

#include <functional>

extern "C" {
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
}

#include "Compat.hh"

#define DEFAULT_PLACEMENT PANEL_TOP
#define DEFAULT_BACKGROUND "SolidRaised #ffffff #eeeeee #cccccc"
#define DEFAULT_HEIGHT 24
#define DEFAULT_SEPARATOR "Solid #aaaaaa 1x24"

#define DEFAULT_COLOR "#000000"
#define DEFAULT_FONT "-misc-fixed-medium-r-*-*-13-*-*-*-*-*-iso10646-*"
#define DEFAULT_FONT_FOC "-misc-fixed-bold-r-*-*-13-*-*-*-*-*-iso10646-*"
#define DEFAULT_FONT_ICO "-misc-fixed-medium-o-*-*-13-*-*-*-*-*-iso10646-*"

#define DEFAULT_BAR_BORDER "black"
#define DEFAULT_BAR_FILL "grey50"

/**
 * Client state, used for selecting correct theme data for the client
 * list.
 */
enum ClientState {
    CLIENT_STATE_FOCUSED,
    CLIENT_STATE_UNFOCUSED,
    CLIENT_STATE_ICONIFIED,
    CLIENT_STATE_NO
};

/**
 * Widget size unit.
 */
enum WidgetUnit {
    WIDGET_UNIT_PIXELS,
    WIDGET_UNIT_PERCENT,
    WIDGET_UNIT_REQUIRED,
    WIDGET_UNIT_REST,
    WIDGET_UNIT_TEXT_WIDTH
};

/**
 * Panel placement.
 */
enum PanelPlacement {
    PANEL_TOP,
    PANEL_BOTTOM
};

/** pekwm configuration file. */
static std::string _pekwm_config_file;

/** empty string, used as default return value. */
static std::string _empty_string;
/** empty string, used as default return value. */
static std::wstring _empty_wstring;

static Util::StringMap<PanelPlacement> panel_placement_map =
    {{"", PANEL_TOP},
     {"TOP", PANEL_TOP},
     {"BOTTOM", PANEL_BOTTOM}};

/** static pekwm resources, accessed via the pekwm namespace. */
static FontHandler* _font_handler = nullptr;
static ImageHandler* _image_handler = nullptr;
static TextureHandler* _texture_handler = nullptr;

namespace pekwm
{
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

/**
 * Size request for widget.
 */
class SizeReq {
public:
    SizeReq(const std::wstring &text)
        : _unit(WIDGET_UNIT_TEXT_WIDTH),
          _size(0),
          _text(text)
    {
    }

    SizeReq(const SizeReq& size_req)
        : SizeReq(size_req._unit, size_req._size)
    {
        _text = size_req._text;
    }

    SizeReq(WidgetUnit unit, uint size)
        : _unit(unit),
          _size(size)
    {
    }

    enum WidgetUnit getUnit(void) const { return _unit; }
    uint getSize(void) const { return _size; }
    const std::wstring& getText(void) const { return _text; }

private:
    WidgetUnit _unit;
    uint _size;
    std::wstring _text;
};

/**
 * Widgets to display.
 */
class WidgetConfig {
public:
    WidgetConfig(const std::string& name, std::vector<std::string> args,
                 const SizeReq& size_req, uint interval_s = UINT_MAX,
                 const CfgParser::Entry* section = nullptr);
    WidgetConfig(const WidgetConfig& cfg);
    ~WidgetConfig(void);

    const std::string& getName(void) const { return _name; }
    const std::string& getArg(uint arg) const
    {
        return arg < _args.size() ? _args[arg] : _empty_string;
    }
    const SizeReq& getSizeReq(void) const { return _size_req; }
    uint getIntervalS(void) const { return _interval_s; }

    const CfgParser::Entry* getCfgSection(void) const { return _section; }

private:
    /** Widget type name. */
    std::string _name;
    /** Widget arguments (if any). */
    std::vector<std::string> _args;
    /** Requested size of widget. */
    SizeReq _size_req;
    /** Refresh interval of widgets, set to UINT_MAX for non time
        based widgets. */
    uint _interval_s;
    /** Configuration section, accessible for widget-specific
        configuration. */
    CfgParser::Entry* _section;
};

WidgetConfig::WidgetConfig(const std::string& name,
                           std::vector<std::string> args,
                           const SizeReq& size_req, uint interval_s,
                           const CfgParser::Entry* section)
    : _name(name),
      _args(args),
      _size_req(size_req),
      _interval_s(interval_s),
      _section(nullptr)
{
    if (section) {
        _section = new CfgParser::Entry(*section);
    }
}

WidgetConfig::WidgetConfig(const WidgetConfig& cfg)
    : _size_req(cfg._size_req)
{
    _name = cfg._name;
    _args = cfg._args;
    _interval_s = cfg._interval_s;
    if (cfg._section) {
        _section = new CfgParser::Entry(*cfg._section);
    } else {
        _section = nullptr;
    }
}

WidgetConfig::~WidgetConfig(void)
{
    delete _section;
}

/**
 * Configuration for commands to be run at given intervals to
 * collect data.
 */
class CommandConfig {
public:
    CommandConfig(const std::string& command,
                  uint interval_s);
    ~CommandConfig(void);

    const std::string& getCommand(void) const { return _command; }
    uint getIntervalS(void) const { return _interval_s; }

private:
    /** Command to run (using the shell) */
    std::string _command;
    /** Interval between runs, not including run time. */
    uint _interval_s;
};

CommandConfig::CommandConfig(const std::string& command,
                             uint interval_s)
    : _command(command),
      _interval_s(interval_s)
{
}

CommandConfig::~CommandConfig(void)
{
}

/**
 * Configuration for panel, read from ~/.pekwm/panel by default.
 */
class PanelConfig {
public:
    typedef std::vector<CommandConfig> command_config_vector;
    typedef command_config_vector::const_iterator command_config_it;

    typedef std::vector<WidgetConfig> widget_config_vector;
    typedef widget_config_vector::const_iterator widget_config_it;

    PanelConfig(void);
    ~PanelConfig(void);

    bool load(const std::string &panel_file);

    PanelPlacement getPlacement(void) const { return _placement; }
    int getHead(void) const { return _head; }

    uint getRefreshIntervalS(void) const { return _refresh_interval_s; }

    command_config_it commandsBegin(void) const { return _commands.begin(); }
    command_config_it commandsEnd(void) const { return _commands.end(); }

    widget_config_it widgetsBegin(void) const { return _widgets.begin(); }
    widget_config_it widgetsEnd(void) const { return _widgets.end(); }

private:
    void loadPanel(CfgParser::Entry *section);
    void loadCommands(CfgParser::Entry *section);
    void loadWidgets(CfgParser::Entry *section);
    void addWidget(const std::string& name,
                   const SizeReq& size_req, uint interval,
                   const std::string& args_str,
                   const CfgParser::Entry* section);
    SizeReq parseSize(const std::string& size);
    uint calculateRefreshIntervalS(void) const;

private:
    /** Position of panel. */
    PanelPlacement _placement;
    /** Panel head, -1 for stretch all heads which is default. */
    int _head;

    /** List of commands to run. */
    command_config_vector _commands;
    /** List of widgets to instantiate. */
    std::vector<WidgetConfig> _widgets;
    /** At what given interval is refresh required at a minimum. */
    uint _refresh_interval_s;
};

PanelConfig::PanelConfig(void)
    : _placement(DEFAULT_PLACEMENT),
      _head(-1)
{
}

PanelConfig::~PanelConfig(void)
{
}

bool
PanelConfig::load(const std::string &panel_file)
{
    CfgParser cfg;
    if (! cfg.parse(panel_file, CfgParserSource::SOURCE_FILE, true)) {
        return false;
    }

    auto root = cfg.getEntryRoot();
    loadPanel(root->findSection("PANEL"));
    loadCommands(root->findSection("COMMANDS"));
    loadWidgets(root->findSection("WIDGETS"));
    _refresh_interval_s = calculateRefreshIntervalS();
    return true;
}

void
PanelConfig::loadPanel(CfgParser::Entry *section)
{
    if (section == nullptr) {
        return;
    }

    std::string placement;
    std::vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyString("PLACEMENT", placement, "TOP"));
    keys.push_back(new CfgParserKeyNumeric<int>("HEAD", _head, -1));
    section->parseKeyValues(keys.begin(), keys.end());
    std::for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    _placement = panel_placement_map.get(placement);
}

void
PanelConfig::loadCommands(CfgParser::Entry *section)
{
    _commands.clear();
    if (section == nullptr) {
        return;
    }

    auto it = section->begin();
    for (; it != section->end(); ++it) {
        uint interval = UINT_MAX;

        if ((*it)->getSection()) {
            std::vector<CfgParserKey*> keys;
            keys.push_back(new CfgParserKeyNumeric<uint>("INTERVAL",
                                                         interval,
                                                         UINT_MAX));
            (*it)->getSection()->parseKeyValues(keys.begin(), keys.end());
            std::for_each(keys.begin(), keys.end(),
                          Util::Free<CfgParserKey*>());
        }
        _commands.push_back(CommandConfig((*it)->getValue(), interval));
    }
}

void
PanelConfig::loadWidgets(CfgParser::Entry *section)
{
    _widgets.clear();
    if (section == nullptr) {
        return;
    }

    auto it = section->begin();
    for (; it != section->end(); ++it) {
        uint interval = UINT_MAX;
        std::string size = "REQUIRED";

        auto w_section = (*it)->getSection();
        if (w_section) {
            std::vector<CfgParserKey*> keys;
            keys.push_back(new CfgParserKeyNumeric<uint>("INTERVAL",
                                                         interval, UINT_MAX));
            keys.push_back(new CfgParserKeyString("SIZE", size, "REQUIRED"));
            w_section->parseKeyValues(keys.begin(), keys.end());
            std::for_each(keys.begin(), keys.end(),
                          Util::Free<CfgParserKey*>());
        }

        auto size_req = parseSize(size);
        addWidget((*it)->getName(), size_req, interval, (*it)->getValue(),
                  w_section);
    }
}

void
PanelConfig::addWidget(const std::string& name,
                       const SizeReq& size_req, uint interval,
                       const std::string& args_str,
                       const CfgParser::Entry* section)
{
    std::vector<std::string> args;
    if (! args_str.empty()) {
        args.push_back(args_str);
    }
    _widgets.push_back(WidgetConfig(name, args, size_req, interval, section));
}

SizeReq
PanelConfig::parseSize(const std::string& size)
{
    std::vector<std::string> toks;
    Util::splitString(size, toks, " \t", 2);
    if (toks.size() == 1) {
        if (strcasecmp("REQUIRED", toks[0].c_str()) == 0) {
            return SizeReq(WIDGET_UNIT_REQUIRED, 0);
        } else if (toks[0] == "*") {
            return SizeReq(WIDGET_UNIT_REST, 0);
        }
    } else if (toks.size() == 2) {
        if (strcasecmp("PIXELS", toks[0].c_str()) == 0) {
            return SizeReq(WIDGET_UNIT_PIXELS, atoi(toks[1].c_str()));
        } else if (strcasecmp("PERCENT", toks[0].c_str()) == 0) {
            return SizeReq(WIDGET_UNIT_PERCENT, atoi(toks[1].c_str()));
        } else if (strcasecmp("TEXTWIDTH", toks[0].c_str()) == 0) {
            return SizeReq(Charset::to_wide_str(toks[1]));
        }
    }

    USER_WARN("failed to parse size: " << size);
    return SizeReq(WIDGET_UNIT_REQUIRED, 0);
}

uint
PanelConfig::calculateRefreshIntervalS(void) const
{
    uint min = UINT_MAX;
    for (auto it : _commands) {
        if (it.getIntervalS() < min) {
            min = it.getIntervalS();
        }
    }
    auto w_it = _widgets.begin();
    for (; w_it != _widgets.end(); ++w_it) {
        if (w_it->getIntervalS() < min) {
            min = w_it->getIntervalS();
        }
    }
    return min;
}

class FieldObservation : public Observation
{
public:
    FieldObservation(const std::string field);
    virtual ~FieldObservation(void);

    const std::string& getField(void) const { return _field; }

private:
    std::string _field;
};

FieldObservation::FieldObservation(const std::string field)
    : _field(field)
{
}

FieldObservation::~FieldObservation(void)
{
}

/**
 * Collection of data from external commands.
 *
 * Commands output is collected in the below format at the specified
 * interval.
 *
 * key data
 *
 */
class ExternalCommandData : public Observable
{
public:
    class CommandProcess
    {
    public:
        CommandProcess(const std::string& command, uint interval_s);
        ~CommandProcess(void);

        int getFd(void) const { return _fd; }
        pid_t getPid(void) const { return _pid; }
        std::string& getBuf(void) { return _buf; }

        bool start(void)
        {
            int fd[2];
            int ret = pipe(fd);
            if (ret == -1) {
                ERR("pipe failed due to: " << strerror(errno));
                return false;
            }

            _pid = fork();
            if (_pid == -1) {
                close(fd[0]);
                close(fd[1]);
                ERR("fork failed due to: " << strerror(errno));
                return false;
            } else if (_pid == 0) {
                // child, dup write end of file descriptor to stdout
                dup2(fd[1], STDOUT_FILENO);

                close(fd[0]);
                close(fd[1]);

                execlp("/bin/sh", "sh", "-c", _command.c_str(), nullptr);

                ERR("failed to execute: " << _command);

                close(STDOUT_FILENO);
                exit(1);
            }

            // parent, close write end just going to read
            _fd = fd[0];
            close(fd[1]);
            Util::setNonBlock(_fd);
            if (Debug::isLevel(Debug::LEVEL_TRACE)) {
                std::ostringstream msg;
                msg << "pid " << _pid << " started with fd " << _fd;
                msg << " for command " << _command;
                TRACE(msg.str());
            }
            return true;
        }

        bool checkInterval(struct timespec *now)
        {
            return now->tv_sec >= _next_interval.tv_sec;
        }

        void reset(void)
        {
            _pid = -1;
            if (_fd != -1) {
                close(_fd);
            }
            _fd = -1;
            _buf.clear();

            int ret = clock_gettime(CLOCK_MONOTONIC, &_next_interval);
            assert(ret == 0);
            _next_interval.tv_sec += _interval_s;
        }

    private:
        std::string _command;
        uint _interval_s;
        struct timespec _next_interval;

        pid_t _pid;
        int _fd;
        std::string _buf;
    };

    ExternalCommandData(const PanelConfig& cfg);
    ~ExternalCommandData(void);

    const std::wstring& get(const std::string& field) const
    {
        auto it = _fields.find(field);
        return it == _fields.end() ? _empty_wstring : it->second;
    }

    void refresh(std::function<void(int)> addFd)
    {
        struct timespec now;
        int ret = clock_gettime(CLOCK_MONOTONIC, &now);
        assert(ret == 0);

        auto it = _command_processes.begin();
        for (; it != _command_processes.end(); ++it) {
            if (it->getPid() == -1
                && it->checkInterval(&now)
                && it->start()) {
                addFd(it->getFd());
            }
        }
    }

    bool input(int fd)
    {
        char buf[1024];
        ssize_t nread = read(fd, buf, sizeof(buf));
        if (nread < 1) {
            if (nread == -1) {
                TRACE("failed to read from " << fd << ": " << strerror(errno));
            }
            return false;
        }

        auto it = _command_processes.begin();
        for (; it != _command_processes.end(); ++it) {
            if (it->getFd() == fd) {
                append(it->getBuf(), buf, nread);
                break;
            }
        }

        return true;
    }

    void done(pid_t pid, std::function<void(int)> removeFd)
    {
        auto it = _command_processes.begin();
        for (; it != _command_processes.end(); ++it) {
            if (it->getPid() == pid) {
                while (input(it->getFd())) {
                    // read data left in pipe if any
                }
                parseOutput(it->getBuf());
                removeFd(it->getFd());

                // clean up state, resetting timer and pid/fd
                it->reset();
                break;
            }
        }
    }

private:
    void append(std::string &buf, char *data, size_t size)
    {
        buf.append(data, data + size);
        auto pos = buf.find('\n');
        while (pos != std::string::npos) {
            auto line = buf.substr(0, pos);
            parseLine(line);
            buf.erase(0, pos + 1);
            pos = buf.find('\n');
        }
    }

    void parseOutput(const std::string& buf)
    {
        std::vector<std::string> lines;
        Util::splitString(buf, lines, "\n");
        for (auto line : lines) {
            parseLine(line);
        }
    }

    void parseLine(const std::string& line)
    {
        std::vector<std::string> field_value;
        if (Util::splitString(line, field_value, " \t", 2) == 2) {
            _fields[field_value[0]] = Charset::to_wide_str(field_value[1]);
            FieldObservation field_obs(field_value[0]);
            notifyObservers(&field_obs);
        }
    }

private:
    const PanelConfig& _cfg;

    std::map<std::string, std::wstring> _fields;
    std::vector<CommandProcess> _command_processes;
};

ExternalCommandData::CommandProcess::CommandProcess(const std::string& command,
                                                    uint interval_s)
    : _command(command),
      _interval_s(interval_s),
      _pid(-1),
      _fd(-1)
{
    // set next interval to last second to ensure immediate
    // execution
    int ret = clock_gettime(CLOCK_MONOTONIC, &_next_interval);
    assert(ret == 0);
    _next_interval.tv_sec--;
}

ExternalCommandData::CommandProcess::~CommandProcess(void)
{
    reset();
}

class ClientInfo : public NetWMStates {
public:
    ClientInfo(Window window);
    virtual ~ClientInfo(void);

    Window getWindow(void) const { return _window; }
    const std::wstring& getName(void) const { return _name; }
    const Geometry& getGeometry(void) const { return _gm; }
    PImage *getIcon(void) const { return _icon; }

    bool displayOn(uint workspace) const
    {
        return sticky || this->_workspace == workspace;
    }

    bool handlePropertyNotify(XPropertyEvent *ev);

private:
    std::wstring readName(void);

    Geometry readGeometry(void)
    {
        return Geometry();
    }

    uint readWorkspace(void)
    {
        Cardinal workspace;
        if (! X11::getCardinal(_window, NET_WM_DESKTOP, workspace)) {
            TRACE("failed to read _NET_WM_DESKTOP on " << _window
                  << " using 0");
            return 0;
        }
        return workspace;
    }

private:
    Window _window;
    std::wstring _name;
    Geometry _gm;
    uint _workspace;
    PImageIcon *_icon;
};

ExternalCommandData::ExternalCommandData(const PanelConfig& cfg)
    : _cfg(cfg)
{
    auto it = _cfg.commandsBegin();
    for (; it != _cfg.commandsEnd(); ++it) {
        _command_processes.push_back(CommandProcess(it->getCommand(),
                                                    it->getIntervalS()));
    }
}

ExternalCommandData::~ExternalCommandData(void)
{
}

ClientInfo::ClientInfo(Window window)
    : _window(window)
{
    X11::selectInput(_window, PropertyChangeMask);

    _name = readName();
    _gm = readGeometry();
    _workspace = readWorkspace();
    X11Util::readEwmhStates(_window, *this);
    _icon = PImageIcon::newFromWindow(_window);
}

ClientInfo::~ClientInfo(void)
{
    if (_icon) {
        delete _icon;
    }
}

bool
ClientInfo::handlePropertyNotify(XPropertyEvent *ev)
{
    if (ev->atom == X11::getAtom(NET_WM_NAME)
        || ev->atom == XA_WM_NAME) {
        _name = readName();
    } else if (ev->atom == X11::getAtom(NET_WM_DESKTOP)) {
        _workspace = readWorkspace();
    } else if (ev->atom == X11::getAtom(STATE)) {
        X11Util::readEwmhStates(_window, *this);
    } else {
        return false;
    }
    return true;
}

std::wstring
ClientInfo::readName(void)
{
    std::string name;
    if (X11::getUtf8String(_window, NET_WM_VISIBLE_NAME, name)) {
        return Charset::from_utf8_str(name);
    }
    if (X11::getUtf8String(_window, NET_WM_NAME, name)) {
        return Charset::from_utf8_str(name);
    }
    if (X11::getTextProperty(_window, XA_WM_NAME, name)) {
        return Charset::to_wide_str(name);
    }
    return _empty_wstring;
}

/**
 * Current window manager state.
 */
class WmState : public Observable
{
public:
    class XROOTPMAP_ID_Changed : public Observation {
    };

    class PEKWM_THEME_Changed : public Observation {
    };

    typedef std::vector<ClientInfo*> client_info_vector;
    typedef client_info_vector::const_iterator client_info_it;

    WmState(void);
    virtual ~WmState(void);

    void read(void)
    {
        readActiveWorkspace();
        readActiveWindow();
        readClientListStacking();
        readDesktopNames();
    }

    uint getActiveWorkspace(void) const { return _workspace; }
    const std::wstring& getWorkspaceName(uint num) const {
        if (num < _desktop_names.size()) {
            return _desktop_names[num];
        }
        return _empty_wstring;
    }
    Window getActiveWindow(void) const { return _active_window; }
    ClientInfo *findClientInfo(Window win) const;

    uint numClients(void) const { return _clients.size(); }
    client_info_it clientsBegin(void) const { return _clients.begin(); }
    client_info_it clientsEnd(void) const { return _clients.end(); }

    bool handlePropertyNotify(XPropertyEvent *ev);

private:
    ClientInfo* findClientInfo(Window win,
                               const std::vector<ClientInfo*> &clients) const;
    ClientInfo* popClientInfo(Window win, std::vector<ClientInfo*> &clients);

    bool readActiveWorkspace(void);
    bool readActiveWindow(void);
    bool readClientListStacking(void);
    bool readDesktopNames(void);

private:
    Window _active_window;
    uint _workspace;
    client_info_vector _clients;
    std::vector<std::wstring> _desktop_names;

    XROOTPMAP_ID_Changed _xrootpmap_id_changed;
    PEKWM_THEME_Changed _pekwm_theme_changed;
};

WmState::WmState(void)
    : _active_window(None),
      _workspace(0)
{
    read();
}

WmState::~WmState(void)
{
    for (auto it : _clients) {
        delete it;
    }
}

ClientInfo*
WmState::findClientInfo(Window win) const
{
    return findClientInfo(win, _clients);
}

bool
WmState::handlePropertyNotify(XPropertyEvent *ev)
{
    bool updated = false;
    Observation *observation = nullptr;

    if (ev->window == X11::getRoot()) {
        if (ev->atom == X11::getAtom(NET_CURRENT_DESKTOP)) {
            updated = readActiveWorkspace();
        } else if (ev->atom == X11::getAtom(NET_ACTIVE_WINDOW)) {
            updated = readActiveWindow();
        } else if (ev->atom == X11::getAtom(NET_CLIENT_LIST)) {
            updated = readClientListStacking();
        } else if (ev->atom == X11::getAtom(XROOTPMAP_ID)) {
            observation = &_xrootpmap_id_changed;
        } else if (ev->atom == X11::getAtom(PEKWM_THEME)) {
            observation = &_pekwm_theme_changed;
        }
    } else {
        auto client_info = findClientInfo(ev->window, _clients);
        if (client_info != nullptr) {
            updated = client_info->handlePropertyNotify(ev);
        }
    }

    if (updated || observation) {
        notifyObservers(observation);
    }

    return updated;
}

ClientInfo*
WmState::findClientInfo(Window win,
                        const std::vector<ClientInfo*> &clients) const
{
    auto it = clients.begin();
    for (; it != clients.end(); ++it) {
        if ((*it)->getWindow() == win) {
            return *it;
        }
    }
    return nullptr;
}

ClientInfo*
WmState::popClientInfo(Window win, std::vector<ClientInfo*> &clients)
{
    auto it = clients.begin();
    for (; it != clients.end(); ++it) {
        if ((*it)->getWindow() == win) {
            auto client_info = *it;
            clients.erase(it);
            return client_info;
        }
    }
    return nullptr;
}

bool
WmState::readActiveWorkspace(void)
{
    Cardinal workspace;
    if (! X11::getCardinal(X11::getRoot(), NET_CURRENT_DESKTOP,
                           workspace)) {
        TRACE("failed to read _NET_CURRENT_DESKTOP, setting to 0");
        _workspace = 0;
        return false;
    }
    _workspace = workspace;
    return true;
}

bool
WmState::readActiveWindow(void)
{
    if (! X11::getWindow(X11::getRoot(),
                         NET_ACTIVE_WINDOW, _active_window)) {
        TRACE("failed to read _NET_ACTIVE_WINDOW, setting to None");
        _active_window = None;
        return false;
    }
    return true;
}

bool
WmState::readClientListStacking(void)
{
    ulong actual;
    Window *windows;
    if (! X11::getProperty(X11::getRoot(),
                           X11::getAtom(NET_CLIENT_LIST),
                           XA_WINDOW, 0,
                           reinterpret_cast<uchar**>(&windows), &actual)) {
        TRACE("failed to read _NET_CLIENT_LIST");
        return false;
    }

    client_info_vector old_clients = _clients;
    _clients.clear();
    for (uint i = 0; i < actual; i++) {
        auto client_info = popClientInfo(windows[i], old_clients);
        if (client_info == nullptr) {
            _clients.push_back(new ClientInfo(windows[i]));
        } else {
            _clients.push_back(client_info);
        }
    }

    for (auto it : old_clients) {
        delete it;
    }

    X11::free(windows);

    TRACE("read _NET_CLIENT_LIST, " << actual << " windows");
    return true;
}

bool
WmState::readDesktopNames(void)
{
    _desktop_names.clear();

    uchar *data;
    ulong data_length;
    if (! X11::getProperty(X11::getRoot(), X11::getAtom(NET_DESKTOP_NAMES),
                           X11::getAtom(UTF8_STRING), 0, &data, &data_length)) {
        return false;
    }

    char *name = reinterpret_cast<char*>(data);
    for (ulong i = 0; i < data_length; ) {
        _desktop_names.push_back(Charset::from_utf8_str(name));
        int name_len = strlen(name);
        i += name_len + 1;
        name += name_len + 1;
    }
    X11::free(data);

    return true;
}

/**
 * Theme for the panel and its widgets.
 */
class PanelTheme {
public:
    PanelTheme(void);
    ~PanelTheme(void);

    void load(const std::string &theme_dir, const std::string& theme_path);
    void unload(void);

    void setIconPath(const std::string& config_path,
                     const std::string& theme_path);

    uint getHeight(void) const { return _height; }

    PFont *getFont(ClientState state) const { return _fonts[state]; }
    PTexture *getBackground(void) const { return _background; }
    PTexture *getSep(void) const { return _sep; }
    PTexture *getHandle(void) const { return _handle; }

    // Bar specific themeing
    XColor *getBarBorder(void) const { return _bar_border; }
    XColor *getBarFill(void) const { return _bar_fill; }

private:
    void loadState(CfgParser::Entry *section, ClientState state);
    void check(void);

private:
    /** Panel height, can be fixed or calculated from font size */
    uint _height;
    /** Set to true after load has been called. */
    bool _loaded;

    /** Panel background texture. */
    PTexture *_background;
    /** If < 255 (dervied from 0-100%) panel background is blended on
        top of background image. */
    uchar _background_opacity;
    /** Texture rendered between widgets in the bar. */
    PTexture *_sep;
    /** Texture rendered at the left and right of the bar, optional. */
    PTexture *_handle;
    PFont* _fonts[CLIENT_STATE_NO];
    PFont::Color* _colors[CLIENT_STATE_NO];

    /** Bar border color. */
    XColor *_bar_border;
    /** Bar fill color. */
    XColor *_bar_fill;
};

PanelTheme::PanelTheme(void)
    : _height(DEFAULT_HEIGHT),
      _loaded(false),
      _background(nullptr),
      _background_opacity(255),
      _sep(nullptr),
      _handle(nullptr),
      _bar_border(nullptr),
      _bar_fill(nullptr)
{
    memset(_fonts, 0, sizeof(_fonts));
    memset(_colors, 0, sizeof(_colors));
}

PanelTheme::~PanelTheme(void)
{
    unload();
}

void
PanelTheme::load(const std::string &theme_dir, const std::string& theme_path)
{
    unload();

    CfgParser theme;
    theme.setVar("$THEME_DIR", theme_dir);
    if (! theme.parse(theme_path, CfgParserSource::SOURCE_FILE, true)) {
        check();
        return;
    }
    auto section = theme.getEntryRoot()->findSection("PANEL");
    if (section == nullptr) {
        check();
        return;
    }

    std::string background, separator, handle, bar_border, bar_fill;
    uint opacity;
    std::vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyString("BACKGROUND",
                                          background, DEFAULT_BACKGROUND));
    keys.push_back(new CfgParserKeyNumeric<uint>("BACKGROUNDOPACITY",
                                                 opacity, 100));
    keys.push_back(new CfgParserKeyNumeric<uint>("HEIGHT",
                                                 _height, DEFAULT_HEIGHT));
    keys.push_back(new CfgParserKeyString("SEPARATOR",
                                          separator, DEFAULT_SEPARATOR));
    keys.push_back(new CfgParserKeyString("HANDLE", handle, ""));
    keys.push_back(new CfgParserKeyString("BARBORDER", bar_border,
                                          DEFAULT_BAR_BORDER));
    keys.push_back(new CfgParserKeyString("BARFILL", bar_fill,
                                          DEFAULT_BAR_FILL));
    section->parseKeyValues(keys.begin(), keys.end());
    std::for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    auto ih = pekwm::imageHandler();
    ih->path_clear();
    ih->path_push_back(theme_dir + "/");

    auto th = pekwm::textureHandler();
    _background = th->getTexture(background);
    _background_opacity = 255.0 * opacity / 100.0;
    _sep = th->getTexture(separator);
    if (! handle.empty()) {
        _handle = th->getTexture(handle);
    }
    _bar_border = X11::getColor(bar_border);
    _bar_fill = X11::getColor(bar_fill);

    loadState(section->findSection("FOCUSED"), CLIENT_STATE_FOCUSED);
    loadState(section->findSection("UNFOCUSED"), CLIENT_STATE_UNFOCUSED);
    loadState(section->findSection("ICONIFIED"), CLIENT_STATE_ICONIFIED);

    check();
}

void
PanelTheme::unload(void)
{
    if (! _loaded) {
        return;
    }

    _height = DEFAULT_HEIGHT;
    X11::returnColor(_bar_border);
    _bar_border = nullptr;
    X11::returnColor(_bar_fill);
    _bar_fill = nullptr;
    auto th = pekwm::textureHandler();
    if (_handle) {
        th->returnTexture(_handle);
        _handle = nullptr;
    }
    th->returnTexture(_sep);
    _sep = nullptr;
    th->returnTexture(_background);
    _background = nullptr;

    auto fh = pekwm::fontHandler();
    for (int i = 0; i < CLIENT_STATE_NO; i++) {
        fh->returnFont(_fonts[i]);
        _fonts[i] = nullptr;
        fh->returnColor(_colors[i]);
        _colors[i] = nullptr;
    }
    _loaded = false;
}

void
PanelTheme::loadState(CfgParser::Entry *section, ClientState state)
{
    if (section == nullptr) {
        return;
    }

    std::string font, color;
    std::vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyString("FONT", font, DEFAULT_FONT));
    keys.push_back(new CfgParserKeyString("COLOR", color, "#000000"));
    section->parseKeyValues(keys.begin(), keys.end());
    std::for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    _fonts[state] = pekwm::fontHandler()->getFont(font);
    _colors[state] = pekwm::fontHandler()->getColor(color);
}

void
PanelTheme::setIconPath(const std::string& config_path,
                        const std::string& theme_path)
{
    auto ih = pekwm::imageHandler();
    ih->path_push_back(DATADIR "/pekwm/icons/");
    ih->path_push_back(config_path);
    ih->path_push_back(theme_path);
}

void
PanelTheme::check(void)
{
    auto fh = pekwm::fontHandler();
    for (int i = 0; i < CLIENT_STATE_NO; i++) {
        if (_fonts[i] == nullptr) {
            _fonts[i] = fh->getFont(DEFAULT_FONT "#Center");
        }
        if (_colors[i] == nullptr) {
            _colors[i] = fh->getColor(DEFAULT_COLOR);
        }
    }

    auto th = pekwm::textureHandler();
    if (_background == nullptr) {
        _background = th->getTexture(DEFAULT_BACKGROUND);
    }
    if (_sep == nullptr) {
        _sep = th->getTexture(DEFAULT_SEPARATOR);
    }

    _background->setOpacity(_background_opacity);
    for (int i = 0; i < CLIENT_STATE_NO; i++) {
        _fonts[i]->setColor(_colors[i]);
    }

    if (_bar_border == nullptr) {
        _bar_border = X11::getColor(DEFAULT_BAR_BORDER);
    }
    if (_bar_fill == nullptr) {
        _bar_fill = X11::getColor(DEFAULT_BAR_FILL);
    }

    _loaded = true;
}

static void
loadTheme(PanelTheme& theme, const std::string& pekwm_config_file)
{
    CfgParser cfg;
    cfg.parse(pekwm_config_file, CfgParserSource::SOURCE_FILE, true);

    std::string config_file;
    std::string theme_dir, theme_variant, theme_path;
    Util::getThemeDir(cfg.getEntryRoot(), theme_dir, theme_variant, theme_path);

    theme.load(theme_dir, theme_path);

    std::string icon_path;
    Util::getIconDir(cfg.getEntryRoot(), icon_path);
    theme.setIconPath(icon_path, theme_dir + "/icons/");
}

/**
 * Base class for all widgets displayed on the panel.
 */
class PanelWidget {
public:
    PanelWidget(const PanelTheme &theme, const SizeReq& size_req);
    virtual ~PanelWidget(void);

    bool isDirty(void) const { return _dirty; }
    int getX(void) const { return _x; }
    int getRX(void) const { return _rx; }
    void move(int x) {
        _x = x;
        _rx = x + _width;
    }

    uint getWidth(void) const { return _width; }
    void setWidth(uint width) {
        _width = width;
        _rx = _x + width;
    }

    const SizeReq& getSizeReq(void) const { return _size_req; }
    virtual uint getRequiredSize(void) const { return 0; }

    virtual void click(int, int) { }

    virtual void render(Render& render)
    {
        render.clear(_x, 0, _width, _theme.getHeight());
        _dirty = false;
    }

protected:
    int renderText(Render &rend, PFont *font,
                   int x, const std::wstring& text, uint max_width);

protected:
    const PanelTheme& _theme;
    bool _dirty;

private:
    int _x;
    int _rx;
    uint _width;
    SizeReq _size_req;
};

PanelWidget::PanelWidget(const PanelTheme &theme,
                         const SizeReq& size_req)
    : _theme(theme),
      _dirty(true),
      _x(0),
      _rx(0),
      _width(0),
      _size_req(size_req)
{
}

PanelWidget::~PanelWidget(void)
{
}

int
PanelWidget::renderText(Render &rend, PFont *font,
                        int x, const std::wstring& text, uint max_width)
{
    int y = (_theme.getHeight() - font->getHeight()) / 2;
    return font->draw(rend.getDrawable(), x, y, text, 0, max_width);
}

/**
 * Current Date/Time formatted using strftime.
 */
class DateTimeWidget : public PanelWidget {
public:
    DateTimeWidget(const PanelTheme &theme,
                   const SizeReq& size_req,
                   const std::string &format)
        : PanelWidget(theme, size_req),
          _format(format)
    {
        if (_format.empty()) {
            _format = "%Y-%m-%d %H:%M";
        }
    }

    virtual uint getRequiredSize(void) const override
    {
        std::wstring wtime;
        formatNow(wtime);
        auto font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
        return font->getWidth(L" " + wtime + L" ");
    }

    virtual void render(Render &rend) override
    {
        PanelWidget::render(rend);

        std::wstring wtime;
        formatNow(wtime);
        auto font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
        renderText(rend, font, getX(), wtime, getWidth());

        // always treat date time as dirty, requires redraw up to
        // every second.
        _dirty = true;
    }

private:
    void formatNow(std::wstring &res) const
    {
        time_t now = time(NULL);
        struct tm tm;
        localtime_r(&now, &tm);

        char buf[64];
        strftime(buf, sizeof(buf), _format.c_str(), &tm);
        res = Charset::to_wide_str(buf);
    }

private:
    std::string _format;
};

/**
 * List of Frames/Clients on the current workspace.
 */
class ClientListWidget : public PanelWidget,
                         public Observer {
public:
    class Entry {
    public:
        Entry(const std::wstring& name, ClientState state, int x, Window window,
              PImage *icon)
            : _name(name),
              _state(state),
              _x(x),
              _window(window),
              _icon(icon)
        {
        }

        const std::wstring getName(void) const { return _name; }
        ClientState getState(void) const { return _state; }
        int getX(void) const { return _x; }
        void setX(int x) { _x = x; }
        Window getWindow(void) const { return _window; }
        PImage *getIcon(void) const { return _icon; }

    private:
        std::wstring _name;
        ClientState _state;
        int _x;
        Window _window;
        PImage *_icon;
    };

    ClientListWidget(const PanelTheme& theme,
                     const SizeReq& size_req,
                     WmState& wm_state);
    virtual ~ClientListWidget(void);

    virtual void notify(Observable*, Observation*) override
    {
        _dirty = true;
        update();
    }

    virtual void click(int x, int) override;
    virtual void render(Render &rend) override;

private:
    Window findClientAt(int x)
    {
        for (auto it : _entries) {
            if (x >= it.getX() && x <= (it.getX() + _entry_width)) {
                return it.getWindow();
            }
        }
        return None;
    }

    void update(void);

private:
    WmState& _wm_state;
    int _entry_width;
    std::vector<Entry> _entries;
};

ClientListWidget::ClientListWidget(const PanelTheme& theme,
                                   const SizeReq& size_req,
                                   WmState& wm_state)
    : PanelWidget(theme, size_req),
      _wm_state(wm_state)
{
    _wm_state.addObserver(this);
}

ClientListWidget::~ClientListWidget(void)
{
    _wm_state.removeObserver(this);
}

void
ClientListWidget::click(int x, int)
{
    auto window = findClientAt(x);
    if (window == None) {
        return;
    }

    Cardinal timestamp = 0;
    X11::getCardinal(window, NET_WM_USER_TIME, timestamp);
    TRACE("ClientListWidget activate " << window << " timestamp "
          << timestamp);

    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.serial = 0;
    ev.xclient.send_event = True;
    ev.xclient.message_type = X11::getAtom(NET_ACTIVE_WINDOW);
    ev.xclient.window = window;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 2;
    ev.xclient.data.l[1] = timestamp;
    ev.xclient.data.l[2] = _wm_state.getActiveWindow();

    X11::sendEvent(X11::getRoot(), False,
                    SubstructureRedirectMask|SubstructureNotifyMask, &ev);
}

void
ClientListWidget::render(Render &rend)
{
    PanelWidget::render(rend);

    uint height = _theme.getHeight() - 2;
    for (auto it : _entries) {
        auto icon = it.getIcon();
        int icon_width = icon ? height + 1 : 0;
        int x = getX() + it.getX() + icon_width;
        int entry_width = _entry_width - icon_width;

        auto font = _theme.getFont(it.getState());
        int icon_x = renderText(rend, font, x, it.getName(), entry_width);
        icon_x -= icon_width;

        if (icon) {
            if (icon->getHeight() > height) {
                icon->scale(height, height);
            }
            int icon_y = (height - icon->getHeight()) / 2;
            icon->draw(rend, icon_x, icon_y);
        }
    }
}

void
ClientListWidget::update(void)
{
    _entries.clear();

    uint workspace = _wm_state.getActiveWorkspace();

    {
        auto it = _wm_state.clientsBegin();
        for (; it != _wm_state.clientsEnd(); ++it) {
            if ((*it)->displayOn(workspace)) {
                ClientState state;
                if ((*it)->getWindow() == _wm_state.getActiveWindow()) {
                    state = CLIENT_STATE_FOCUSED;
                } else if ((*it)->hidden) {
                    state = CLIENT_STATE_ICONIFIED;
                } else {
                    state = CLIENT_STATE_UNFOCUSED;
                }
                _entries.emplace_back(Entry((*it)->getName(), state, 0,
                                            (*it)->getWindow(),
                                            (*it)->getIcon()));
            }
        }
    }

    // no clients on active workspace, skip rendering and avoid
    // division by zero.
    if (_entries.empty()) {
        _entry_width = getWidth();
    } else {
        _entry_width = getWidth() / _entries.size();
    }

    {
        int x = 0;
        auto it = _entries.begin();
        for (; it != _entries.end(); ++it) {
            it->setX(x);
            x += _entry_width;
        }
    }
}

/**
 * Widget display a bar filled from 0-100% getting fill percentage
 * from external command data.
 */
class BarWidget : public PanelWidget,
                  public Observer {
public:
    BarWidget(const PanelTheme& theme,
              const SizeReq& size_req,
              ExternalCommandData& ext_data,
              const std::string& field,
              const CfgParser::Entry *section);
    virtual ~BarWidget(void);

    virtual void notify(Observable*, Observation *observation) override
    {
        auto efo = dynamic_cast<FieldObservation*>(observation);
        if (efo != nullptr && efo->getField() == _field) {
            _dirty = true;
        }
    }

    virtual void render(Render &rend) override;

private:
    int getBarFill(float percent) const;
    float getPercent(const std::wstring& str) const;
    void parseColors(const CfgParser::Entry* section);
    void addColor(float percent, XColor* color);

private:
    ExternalCommandData& _ext_data;
    std::string _field;
    std::vector<std::pair<float, XColor*>> _colors;
};

BarWidget::BarWidget(const PanelTheme& theme,
                     const SizeReq& size_req,
                     ExternalCommandData& ext_data,
                     const std::string& field,
                     const CfgParser::Entry *section)
    : PanelWidget(theme, size_req),
      _ext_data(ext_data),
      _field(field)
{
    parseColors(section);
    _ext_data.addObserver(this);
}

BarWidget::~BarWidget(void)
{
    _ext_data.removeObserver(this);
}

void
BarWidget::render(Render &rend)
{
    PanelWidget::render(rend);

    int width = getWidth() - 3;
    int height = _theme.getHeight() - 4;
    rend.setColor(_theme.getBarBorder()->pixel);
    rend.rectangle(getX() + 1, 1, width, height);

    float fill_p = getPercent(_ext_data.get(_field));
    int fill = fill_p * (height - 2);
    rend.setColor(getBarFill(fill_p));
    rend.fill(getX() + 2, 1 + height - fill, width - 1, fill);
}

int
BarWidget::getBarFill(float percent) const
{
    auto it = _colors.rbegin();
    for (; it != _colors.rend(); ++it) {
        if (it->first <= percent) {
            break;
        }
    }

    if (it == _colors.rend()) {
        return _theme.getBarFill()->pixel;
    }
    return it->second->pixel;
}

float
BarWidget::getPercent(const std::wstring& str) const
{
    try {
        float percent = std::stof(str);
        if (percent < 0.0) {
            percent = 0.0;
        } else if (percent > 100.0) {
            percent = 100.0;
        }
        return percent / 100;
    } catch (std::invalid_argument&) {
        return 0.0;
    }
}

void
BarWidget::parseColors(const CfgParser::Entry* section)
{
    auto colors = section->findSection("COLORS");
    if (colors == nullptr) {
        return;
    }

    auto it = colors->begin();
    for (; it != colors->end(); ++it) {
        if (strcasecmp((*it)->getName().c_str(), "PERCENT")
            || ! (*it)->getSection()) {
            continue;
        }

        float percent = getPercent(Charset::to_wide_str((*it)->getValue()));
        auto color = (*it)->getSection()->findEntry("COLOR");
        if (color) {
            addColor(percent, X11::getColor(color->getValue()));
        }
    }
}

void
BarWidget::addColor(float percent, XColor* color)
{
    auto it = _colors.begin();
    for (; it != _colors.end(); ++it) {
        if (percent < it->first) {
            break;
        }
    }

    _colors.insert(it, std::pair<float, XColor*>(percent, color));
}

class TextFormatter
{
public:
    TextFormatter(ExternalCommandData& ext_data, WmState& wm_state);
    ~TextFormatter(void);

    bool referenceWmState(void) const { return _check_wm_state; }
    std::vector<std::string> getFields(void) { return _fields; }

    std::wstring preprocess(const std::wstring& raw_format);
    std::wstring format(const std::wstring& pp_format);

private:
    std::wstring format(const std::wstring& pp_format,
                        std::function<std::wstring(const std::wstring&)> exp);

    std::wstring preprocessVar(const std::wstring& var);
    std::wstring expandVar(const std::wstring& var);

private:
    ExternalCommandData& _ext_data;
    WmState& _wm_state;

    bool _check_wm_state;
    std::vector<std::string> _fields;
};

TextFormatter::TextFormatter(ExternalCommandData& ext_data, WmState& wm_state)
    : _ext_data(ext_data),
      _wm_state(wm_state),
      _check_wm_state(false)
{
}

TextFormatter::~TextFormatter(void)
{
}

/**
 * pre process format string for use with the format command, expands
 * environment variables and other static data.
 */
std::wstring
TextFormatter::preprocess(const std::wstring& raw_format)
{
    return format(raw_format,
                  [this](const std::wstring& buf) {
                      return this->preprocessVar(buf);
                  });
}

/**
 * format previously pre-processed format string expanding external
 * command data and wm state.
 */
std::wstring
TextFormatter::format(const std::wstring& pp_format)
{
    return format(pp_format,
                  [this](const std::wstring& buf) {
                      return this->expandVar(buf);
                  });
}

std::wstring
TextFormatter::format(const std::wstring& pp_format,
                      std::function<std::wstring(const std::wstring&)> exp)
{
    std::wstring formatted;

    bool in_escape = false, in_var = false;
    std::wstring buf;
    for (auto chr : pp_format) {
        if (in_escape) {
            buf += chr;
            in_escape = false;
        } else if (chr == L'\\') {
            in_escape = true;
        } else if (in_var && isspace(chr)) {
            formatted += exp(buf);
            buf = chr;
            in_var = false;
        } else if (! in_var && chr == L'%') {
            if (! buf.empty()) {
                formatted += buf;
                buf = _empty_wstring;
            }
            in_var = true;
        } else {
            buf += chr;
        }
    }
    if (! buf.empty()) {
        if (in_var) {
            formatted += exp(buf);
        } else {
            formatted += buf;
        }
    }

    return formatted;
}

std::wstring
TextFormatter::preprocessVar(const std::wstring& buf)
{
    if (buf.empty()) {
        return L"%";
    }

    switch (buf[0]) {
    case '_': {
        auto key = Charset::to_mb_str(buf.substr(1));
        return Charset::to_wide_str(Util::getEnv(key));
    }
    case ':':
        _check_wm_state = true;
        return L"%" + buf;
    default:
        _fields.push_back(Charset::to_mb_str(buf));
        return L"%" + buf;
    }
}

std::wstring
TextFormatter::expandVar(const std::wstring& buf)
{
    if (buf.empty()) {
        return _empty_wstring;
    }

    if (buf[0] == L':') {
        // window manager state variable
        if (buf == L":CLIENT_NAME:") {
            auto win = _wm_state.getActiveWindow();
            auto client_info = _wm_state.findClientInfo(win);
            return client_info ? client_info->getName() : _empty_wstring;
        } else if (buf == L":WORKSPACE_NUMBER:") {
            auto ws = std::to_string(_wm_state.getActiveWorkspace() + 1);
            return Charset::to_wide_str(ws);
        } else if (buf == L":WORKSPACE_NAME:") {
            return _wm_state.getWorkspaceName(_wm_state.getActiveWorkspace());
        }
        return _empty_wstring;
    }

    // external command data
    auto field = Charset::to_mb_str(buf);
    return _ext_data.get(field);
}

/**
 * Text widget with format string that is able to reference command
 * data, window manager state data and environment variables.
 *
 * Format $external, $:wm, $_env
 */
class TextWidget : public PanelWidget,
                   public Observer {
public:
    TextWidget(const PanelTheme& theme, const SizeReq& size_req,
               ExternalCommandData& _ext_data, WmState& _wm_state,
               const std::string& format);
    virtual ~TextWidget(void);

    virtual void notify(Observable *, Observation *observation) override;
    virtual void render(Render &rend) override;

private:
    ExternalCommandData& _ext_data;
    WmState& _wm_state;
    std::wstring _pp_format;

    bool _check_wm_state;
    std::vector<std::string> _fields;
};

TextWidget::TextWidget(const PanelTheme& theme, const SizeReq& size_req,
                       ExternalCommandData& ext_data, WmState& wm_state,
                       const std::string& format)
    : PanelWidget(theme, size_req),
      _ext_data(ext_data),
      _wm_state(wm_state),
      _check_wm_state(false)
{
    TextFormatter tf(_ext_data, _wm_state);
    _pp_format = tf.preprocess(Charset::to_wide_str(format));
    _check_wm_state = tf.referenceWmState();
    _fields = tf.getFields();

    if (! _fields.empty()) {
        _ext_data.addObserver(this);
    }
    if (_check_wm_state) {
        _wm_state.addObserver(this);
    }
}

TextWidget::~TextWidget(void)
{
    if (_check_wm_state) {
        _wm_state.removeObserver(this);
    }
    if (! _fields.empty()) {
        _ext_data.removeObserver(this);
    }
}

void
TextWidget::notify(Observable *, Observation *observation)
{
    if (_dirty) {
        return;
    }

    auto fo = dynamic_cast<FieldObservation*>(observation);
    if (fo != nullptr) {
        for (auto it : _fields) {
            if (it == fo->getField()) {
                _dirty = true;
                break;
            }
        }
    } else {
        _dirty = true;
    }
}

void
TextWidget::render(Render &rend)
{
    PanelWidget::render(rend);

    TextFormatter tf(_ext_data, _wm_state);
    auto font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
    auto text = tf.format(_pp_format);
    renderText(rend, font, getX(), text, getWidth());
}

/**
 * Display icon based on value from external command.
 */
class IconWidget : public PanelWidget,
                   public Observer {
public:
    IconWidget(const PanelTheme& theme,
               const SizeReq& size_req,
               ExternalCommandData &ext_data,
               const std::string& field,
               const CfgParser::Entry *section);
    virtual ~IconWidget(void);

    virtual void notify(Observable *, Observation *observation) override;
    virtual void render(Render& rend) override;

private:
    void load(void);
    bool loadImage(const std::string& icon_name);
    void parseIcon(const CfgParser::Entry* section);

private:
    ExternalCommandData& _ext_data;
    std::string _field;
    /** icon name, no file extension. */
    std::string _name;
    /** file extension. */
    std::string _ext;

    /** current loaded icon, matching _icon_name. */
    PImage* _icon;
    std::string _icon_name;
};

IconWidget::IconWidget(const PanelTheme& theme,
                       const SizeReq& size_req,
                       ExternalCommandData &ext_data,
                       const std::string& field,
                       const CfgParser::Entry *section)
    : PanelWidget(theme, size_req),
      _ext_data(ext_data),
      _field(field),
      _icon(nullptr)
{
    parseIcon(section);

    _ext_data.addObserver(this);
    load();
}

IconWidget::~IconWidget(void)
{
    if (_icon) {
        pekwm::imageHandler()->returnImage(_icon);
    }
    _ext_data.removeObserver(this);
}

void
IconWidget::notify(Observable *, Observation *observation)
{
    auto fo = dynamic_cast<FieldObservation*>(observation);
    if (fo != nullptr && fo->getField() == _field) {
        _dirty = true;
        load();
    }
}

void
IconWidget::render(Render& rend)
{
    PanelWidget::render(rend);
    if (_icon == nullptr) {
        return;
    }

    uint height = _theme.getHeight() - 2;
    _icon->draw(rend, getX(), 1, height, height);
}

void
IconWidget::load(void)
{
    std::string value;
    if (! _field.empty()) {
        value = Charset::to_mb_str(_ext_data.get(_field));
    }

    if (value.empty() || ! loadImage(_name + "-" + value + _ext)) {
        loadImage(_name + _ext);
    }
}

bool
IconWidget::loadImage(const std::string& icon_name)
{
    if (_icon_name == icon_name) {
        return true;
    }

    auto ih = pekwm::imageHandler();
    if (_icon) {
        ih->returnImage(_icon);
    }
    _icon = ih->getImage(icon_name);
    if (_icon) {
        _icon_name = icon_name;
    } else {
        _icon_name = "";
    }
    return _icon != nullptr;
}

void
IconWidget::parseIcon(const CfgParser::Entry* section)
{
    std::string name;
    std::vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyString("ICON", name));
    section->parseKeyValues(keys.begin(), keys.end());
    std::for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    auto pos = name.find_last_of(".");
    if (pos == std::string::npos) {
        _name = name;
    } else {
        _name = name.substr(0, pos);
        _ext = name.substr(pos, name.size() - pos);
    }
}

/**
 * Widget construction.
 */
class WidgetFactory {
public:
    WidgetFactory(const PanelTheme& theme,
                  ExternalCommandData& ext_data,
                  WmState& wm_state)
        : _theme(theme),
          _ext_data(ext_data),
          _wm_state(wm_state)
    {
    }

    PanelWidget* construct(const WidgetConfig& cfg);

private:
    const PanelTheme& _theme;
    ExternalCommandData& _ext_data;
    WmState& _wm_state;
};

PanelWidget*
WidgetFactory::construct(const WidgetConfig& cfg)
{
    std::string name = cfg.getName();
    Util::to_upper(name);

    if (name == "BAR") {
        auto field = cfg.getArg(0);
        if (field.empty()) {
            USER_WARN("missing required argument to Bar widget");
        } else {
            return new BarWidget(_theme, cfg.getSizeReq(),
                                 _ext_data, field, cfg.getCfgSection());
        }
    } else if (name == "CLIENTLIST") {
        return new ClientListWidget(_theme, cfg.getSizeReq(), _wm_state);
    } else if (name == "DATETIME") {
        auto format = cfg.getArg(0);
        return new DateTimeWidget(_theme, cfg.getSizeReq(), format);
    } else if (name == "ICON") {
        auto field = cfg.getArg(0);
        return new IconWidget(_theme, cfg.getSizeReq(), _ext_data, field,
                              cfg.getCfgSection());
    } else if (name == "TEXT") {
        auto format = cfg.getArg(0);
        if (format.empty()) {
            USER_WARN("missing required argument to Text widget");
        } else {
            return new TextWidget(_theme, cfg.getSizeReq(),
                                  _ext_data, _wm_state, format);
        }
    } else {
        USER_WARN("unknown widget " << cfg.getName());
    }

    return nullptr;
}

/**
 * Widgets in the panel are given a size when configured, can be given
 * in:
 *
 *   * pixels, number of pixels
 *   * percent, percent of the screen width the panel is on.
 *   * required, minimum required size.
 *   * *, all space not occupied by the other widgets. All * share the remaining
 *     space.
 *
 *       required            300px                         *
 * ----------------------------------------------------------------------------
 * | [WorkspaceNumber] | [ExternalData] | [ClientList]                        |
 * ----------------------------------------------------------------------------
 *
 */
class PekwmPanel : public X11App,
                   public Observer {
public:
    PekwmPanel(const PanelConfig &cfg, PanelTheme &theme, XSizeHints *sh);
    virtual ~PekwmPanel(void);

    const PanelConfig& getCfg(void) const { return _cfg; }

    void configure(void);
    void setStrut(void);
    void place(void);
    void render(void);

    virtual void notify(Observable*, Observation *observation) override;
    virtual void refresh(bool timed_out) override;
    virtual void handleEvent(XEvent *ev) override;

private:

    virtual ActionEvent *handleButtonPress(XButtonEvent *ev) override
    {
        auto widget = findWidget(ev->x);
        if (widget != nullptr) {
            widget->click(ev->x - widget->getX(), ev->y - _gm.y);
        }
        return nullptr;
    }

    virtual ActionEvent *handleButtonRelease(XButtonEvent*) override
    {
        return nullptr;
    }

    void handleExpose(XExposeEvent *ev)
    {
        renderPred([ev](PanelWidget *w) {
            return (w->getX() >= ev->x)
                && (w->getRX() <= (ev->x + ev->width));
        });
    }

    void handlePropertyNotify(XPropertyEvent *ev)
    {
        if (_wm_state.handlePropertyNotify(ev)) {
            render();
        }
    }

    virtual void handleFd(int fd) override
    {
        _ext_data.input(fd);
    }

    virtual void handleChildDone(pid_t pid, int) override
    {
        _ext_data.done(pid, [this](int fd) { this->removeFd(fd); });
    }

    virtual void screenChanged(const ScreenChangeNotification&) override
    {
        TRACE("screen geometry updated, resizing");
        place();
        resizeWidgets();
    }

    PanelWidget* findWidget(int x)
    {
        for (auto it : _widgets) {
            if (x >= it->getX() && x <= it->getRX()) {
                return it;
            }
        }
        return nullptr;
    }

    void addWidgets(void)
    {
        WidgetFactory factory(_theme, _ext_data, _wm_state);

        auto it = _cfg.widgetsBegin();
        for (; it != _cfg.widgetsEnd(); ++it) {
            auto widget = factory.construct(*it);
            if (widget == nullptr) {
                USER_WARN("failed to construct widget");
            } else {
                _widgets.push_back(widget);
            }
        }
    }

    void resizeWidgets(void);
    void renderPred(std::function<bool(PanelWidget*)> pred);
    void renderBackground(void);

private:
    const PanelConfig& _cfg;
    PanelTheme& _theme;
    WmState _wm_state;
    std::vector<PanelWidget*> _widgets;
    ExternalCommandData _ext_data;
    Pixmap _pixmap;
};

PekwmPanel::PekwmPanel(const PanelConfig &cfg, PanelTheme &theme,
                       XSizeHints *sh)
    : X11App({sh->x, sh->y,
                static_cast<uint>(sh->width), static_cast<uint>(sh->height)},
        L"", "panel", "pekwm_panel",
        WINDOW_TYPE_DOCK, sh),
      _cfg(cfg),
      _theme(theme),
      _ext_data(cfg),
      _pixmap(X11::createPixmap(sh->width, sh->height))
{
    X11::selectInput(_window,
                     ButtonPressMask|ButtonReleaseMask|
                     ExposureMask|
                     PropertyChangeMask);

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

    _wm_state.addObserver(this);
}

PekwmPanel::~PekwmPanel(void)
{
    _wm_state.removeObserver(this);
    for (auto it : _widgets) {
        delete it;
    }
    X11::freePixmap(_pixmap);
}

void
PekwmPanel::configure(void)
{
    addWidgets();
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
    auto head = X11Util::getHeadGeometry(_cfg.getHead());

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
    renderPred([](PanelWidget *w) { return w->isDirty(); });
}

void
PekwmPanel::notify(Observable*, Observation *observation)
{
    if (dynamic_cast<WmState::PEKWM_THEME_Changed*>(observation)) {
        DBG("reloading theme, _PEKWM_THEME changed");
        loadTheme(_theme, _pekwm_config_file);
        setStrut();
        place();
    }

    if (dynamic_cast<WmState::XROOTPMAP_ID_Changed*>(observation)
        || dynamic_cast<WmState::PEKWM_THEME_Changed*>(observation)) {
        renderBackground();
        renderPred([](PanelWidget*) { return true; });
    }
}

void
PekwmPanel::refresh(bool timed_out)
{
    _ext_data.refresh([this](int fd) { this->addFd(fd); });
    if (timed_out) {
        renderPred([](PanelWidget*) { return true; });
    }
}

void
PekwmPanel::handleEvent(XEvent *ev)
{
    switch (ev->type) {
    case ButtonPress:
        TRACE("ButtonPress");
        handleButtonPress(&ev->xbutton);
        break;
    case ButtonRelease:
        TRACE("ButtonRelease");
        handleButtonRelease(&ev->xbutton);
        break;
    case ConfigureNotify:
        TRACE("ConfigureNotify");
        break;
    case EnterNotify:
        TRACE("EnterNotify");
        break;
    case Expose:
        TRACE("Expose");
        handleExpose(&ev->xexpose);
        break;
    case LeaveNotify:
        TRACE("LeaveNotify");
        break;
    case MapNotify:
        TRACE("MapNotify");
        break;
    case ReparentNotify:
        TRACE("ReparentNotify");
        break;
    case UnmapNotify:
        TRACE("UnmapNotify");
        break;
    case PropertyNotify:
        TRACE("PropertyNotify");
        handlePropertyNotify(&ev->xproperty);
        break;
    case MappingNotify:
        TRACE("MappingNotify");
        break;
    default:
        DBG("UNKNOWN EVENT " << ev->type);
        break;
    }
}

void
PekwmPanel::resizeWidgets(void)
{
    if (_widgets.empty()) {
        return;
    }

    auto sep = _theme.getSep();
    uint num_rest = 0;
    uint width_left = _gm.width - sep->getWidth() * (_widgets.size() - 1);

    auto handle = _theme.getHandle();
    if (handle) {
        width_left -= handle->getWidth() * 2;
    }

    for (auto it : _widgets) {
        switch (it->getSizeReq().getUnit()) {
        case WIDGET_UNIT_PIXELS:
            width_left -= it->getSizeReq().getSize();
            it->setWidth(it->getSizeReq().getSize());
            break;
        case WIDGET_UNIT_PERCENT: {
            uint width =
                _gm.width
                * (static_cast<float>(it->getSizeReq().getSize()) / 100);
            width_left -= width;
            it->setWidth(width);
            break;
        }
        case WIDGET_UNIT_REQUIRED:
            width_left -= it->getRequiredSize();
            it->setWidth(it->getRequiredSize());
            break;
        case WIDGET_UNIT_REST:
            num_rest++;
            break;
        case WIDGET_UNIT_TEXT_WIDTH: {
            auto font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
            uint width = font->getWidth(it->getSizeReq().getText());
            width_left -= width;
            it->setWidth(width);
            break;
        }
        }
    }

    uint x = handle ? handle->getWidth() : 0;
    uint rest = width_left / static_cast<float>(num_rest);
    for (auto it : _widgets) {
        if (it->getSizeReq().getUnit() == WIDGET_UNIT_REST) {
            it->setWidth(rest);
        }
        it->move(x);
        x += it->getWidth() + sep->getWidth();
    }
}

void
PekwmPanel::renderPred(std::function<bool(PanelWidget*)> pred)
{
    if (_widgets.empty()) {
        return;
    }

    auto sep = _theme.getSep();
    auto handle = _theme.getHandle();

    int x = handle ? handle->getWidth() : 0;
    PanelWidget *last_widget = _widgets.back();
    X11Render rend(_window);
    for (auto it : _widgets) {
        bool do_render = pred(it);
        if (do_render) {
            it->render(rend);
        }
        x += it->getWidth();

        if (do_render && it != last_widget) {
            sep->render(rend, x, 0, sep->getWidth(), sep->getHeight());
            x += sep->getWidth();
        }
    }
}

void
PekwmPanel::renderBackground(void)
{
    _theme.getBackground()->render(_pixmap, 0, 0, _gm.width, _gm.height,
                                   _gm.x, _gm.y);
    auto handle = _theme.getHandle();
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

    std::string panel_config = Util::getEnv("HOME") + "/.pekwm/panel";
    if (cfg.load(panel_config)) {
        return true;
    }

    return cfg.load(SYSCONFDIR "/panel");
}

static void init(Display* dpy)
{
    _font_handler = new FontHandler();
    _image_handler = new ImageHandler();
    _texture_handler = new TextureHandler();
}

static void cleanup()
{
    delete _texture_handler;
    delete _image_handler;
    delete _font_handler;
}

static void usage(const char* name, int ret)
{
    std::cout << "usage: " << name << " [-dh]" << std::endl;
    std::cout << " -c --config path    Configuration file" << std::endl;
    std::cout << " -d --display dpy    Display" << std::endl;
    std::cout << " -h --help           Display this information" << std::endl;
    std::cout << " -f --log-file       Set log file." << std::endl;
    std::cout << " -l --log-level      Set log level." << std::endl;
    exit(ret);
}

int main(int argc, char *argv[])
{
    std::string config_file;
    const char* display = NULL;

    static struct option opts[] = {
        {const_cast<char*>("config"), required_argument, nullptr, 'c'},
        {const_cast<char*>("pekwm-config"), required_argument, nullptr, 'C'},
        {const_cast<char*>("display"), required_argument, nullptr, 'd'},
        {const_cast<char*>("help"), no_argument, nullptr, 'h'},
        {const_cast<char*>("log-level"), required_argument, nullptr, 'l'},
        {const_cast<char*>("log-file"), required_argument, nullptr, 'f'},
        {nullptr, 0, nullptr, 0}
    };

    Charset::init();

    int ch;
    while ((ch = getopt_long(argc, argv, "c:C:d:f:hl:", opts, nullptr)) != -1) {
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
                std::cerr << "Failed to open log file " << optarg << std::endl;
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
        config_file = "~/.pekwm/panel";
    }
    if (_pekwm_config_file.empty()) {
        _pekwm_config_file = "~/.pekwm/config";
    }
    Util::expandFileName(config_file);
    Util::expandFileName(_pekwm_config_file);

    auto dpy = XOpenDisplay(display);
    if (! dpy) {
        auto actual_display = display ? display : Util::getEnv("DISPLAY");
        std::cerr << "Can not open display!" << std::endl
                  << "Your DISPLAY variable currently is set to: "
                  << actual_display << std::endl;
        return 1;
    }

    X11::init(dpy, true);
    init(dpy);

    {
        // run in separate scope to get resources cleaned up before
        // X11 and iconv cleanup
        PanelConfig cfg;
        if (loadConfig(cfg, config_file)) {
            PanelTheme theme;
            loadTheme(theme, _pekwm_config_file);

            auto head = X11Util::getHeadGeometry(cfg.getHead());
            XSizeHints normal_hints = {0};
            normal_hints.flags = PPosition|PSize;
            normal_hints.x = head.x;
            normal_hints.width = head.width;
            normal_hints.height = theme.getHeight();

            if (cfg.getPlacement() == PANEL_TOP) {
                normal_hints.y = head.y;
            } else {
                normal_hints.y = head.y + head.height - normal_hints.height;
            }

            PekwmPanel panel(cfg, theme, &normal_hints);
            panel.configure();
            panel.mapWindow();
            panel.render();
            panel.main(cfg.getRefreshIntervalS());
        } else {
            std::cerr << "failed to read panel configuration" << std::endl;
        }
    }

    cleanup();
    X11::destruct();
    Charset::destruct();

    return 0;
}
