//
// pekwm_dialog.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"

#include "CfgParser.hh"
#include "Debug.hh"
#include "FontHandler.hh"
#include "ImageHandler.hh"
#include "TextureHandler.hh"
#include "Theme.hh"
#include "PWinObj.hh"
#include "Util.hh"
#include "x11.hh"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>

extern "C" {
#include <X11/Xutil.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
}

static const uint WIDTH_DEFAULT = 200;
static const uint HEIGHT_DEFAULT = 50;
#define THEME_DEFAULT DATADIR "/pekwm/themes/default/theme"

static int _stop = -1;
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
 * Dialog
 *
 * --------------------------------------------
 * | TITLE                                    |
 * --------------------------------------------
 * | Image if any is displayed here           |
 * |                                          |
 * |                                          |
 * |                                          |
 * --------------------------------------------
 * | Message text goes here                   |
 * |                                          |
 * --------------------------------------------
 * |           [Option1] [Option2]            |
 * --------------------------------------------
 *
 */
class PekwmDialog : public PWinObj {
public:
    class Widget{
    public:
        virtual ~Widget(void)
        {
            if (_window != None) {
                X11::destroyWindow(_window);
            }
        }

        int getX(void) const { return _gm.x; }
        int getY(void) const { return _gm.y; }

        virtual bool setState(Window window, ButtonState state) {
            return false;
        }
        virtual bool click(Window window) { return false; }
        virtual void render(void) = 0;

        virtual void place(int x, int y, uint width)
        {
            _gm.x = x;
            _gm.y = y;
            _gm.width = width;
        }

        /**
         * Get requested width, 0 means adapt to given width.
         */
        virtual uint widthReq(void) const { return 0; }

        /**
         * Get requested height, given the provided width.
         */
        virtual uint heightReq(uint width) const = 0;

    protected:
        Widget(Theme::DialogData& data, PWinObj &parent)
            : _data(data),
              _window(None),
              _parent(parent)
        {
        }

        void setWindow(Window window) { _window = window; }

    protected:
        Theme::DialogData &_data;
        Window _window;
        PWinObj &_parent;
        Geometry _gm;
    };

    class Button : public Widget {
    public:
        Button(Theme::DialogData& data, PWinObj& parent,
               int retcode, const std::wstring& text)
            : Widget(data, parent),
              _retcode(retcode),
              _text(text),
              _font(data.getButtonFont()),
              _state(BUTTON_STATE_FOCUSED)
        {
            XSetWindowAttributes attr;
            attr.background_pixel = X11::getWhitePixel();
            attr.override_redirect = True;
            attr.event_mask =
                ButtonPressMask|ButtonReleaseMask|
                EnterWindowMask|LeaveWindowMask;
            _gm.width = widthReq();
            _gm.height = heightReq(_gm.width);

            setWindow(X11::createWindow(_parent.getWindow(),
                                        0, 0, _gm.width, _gm.height, 0,
                                        CopyFromParent, InputOutput,
                                        CopyFromParent,
                                        CWEventMask|CWOverrideRedirect|
                                        CWBackPixel, &attr));
            X11::mapWindow(_window);
        }

        virtual ~Button(void)
        {
        }

        virtual bool setState(Window window, ButtonState state) override {
            if (window != _window) {
                return false;
            }
            _state = state;
            render();
            return true;
        }

        virtual bool click(Window window) override {
            if (window != _window) {
                return false;
            }
            if (_state == BUTTON_STATE_HOVER
                || _state == BUTTON_STATE_PRESSED) {
                _stop = _retcode;
            }
            return true;
        }

        virtual void place(int x, int y, uint width) override {
            Widget::place(x, y, _gm.width);
            X11::moveWindow(_window, _gm.x, _gm.y);
        }

        virtual uint widthReq(void) const override {
            return _font->getWidth(_text) + _data.padVert();
        }

        virtual uint heightReq(uint width) const override {
            return _font->getHeight() + _data.padHorz();
        }

        virtual void render(void) override {
            X11::clearWindow(_window);
            _data.getButton(_state)->render(_window, 0, 0,
                                            _gm.width, _gm.height);
            _font->setColor(_data.getButtonColor());
            _font->draw(_window,
                        _data.getPad(PAD_LEFT), _data.getPad(PAD_UP),
                        _text);
        }

    private:
        int _retcode;
        std::wstring _text;
        PFont *_font;
        ButtonState _state;
    };

    class ButtonsRow : public Widget
    {
    public:
        ButtonsRow(Theme::DialogData& data, PWinObj& parent,
                   std::vector<std::wstring> options)
            : Widget(data, parent)
        {
            int i = 0;
            for (auto it : options) {
                _buttons.push_back(new Button(_data, parent, i++, it));
            }
        }

        virtual ~ButtonsRow(void)
        {
            for (auto it : _buttons) {
                delete it;
            }
        }

        virtual bool setState(Window window, ButtonState state) override {
            for (auto it : _buttons) {
                if (it->setState(window, state)) {
                    return true;
                }
            }
            return false;
        }

        virtual bool click(Window window) override {
            for (auto it : _buttons) {
                if (it->click(window)) {
                    return true;
                }
            }
            return false;

        }

        virtual void place(int x, int y, uint width) override
        {
            Widget::place(x, y, width);

            // place buttons centered on available width
            uint buttons_width = 0;
            for (auto it : _buttons) {
                buttons_width += it->widthReq();
            }
            buttons_width += _buttons.size() * _data.padHorz();

            x = (width - buttons_width) / 2;
            y += _data.getPad(PAD_UP);
            for (auto it : _buttons) {
                it->place(x, y, width);
                x += it->widthReq() + _data.padHorz();
            }
        }

        virtual uint heightReq(uint width) const override {
            uint height = 0;
            for (auto it : _buttons) {
                uint height_req = it->heightReq(width);
                if (height_req > height) {
                    height = height_req;
                }
            }
            return height + _data.padVert();
        }

        virtual void render(void) override {
            for (auto it : _buttons) {
                it->render();
            }
        }

    private:
        std::vector<Button*> _buttons;
    };

    class Image : public Widget {
    public:
        Image(Theme::DialogData& data, PWinObj& parent, PImage* image)
            : Widget(data, parent),
              _image(image)
        {
        }
        virtual ~Image(void) { }

        virtual uint widthReq(void) const override
        {
            return _image->getWidth();
        }

        virtual uint heightReq(uint width) const override
        {
            // TODO: if width < image width, this should return the
            // scaled image height considering the aspect
            return _image->getHeight();
        }

        virtual void render(void) override
        {
            // render image centered on available width
            uint x = (_gm.width - _image->getWidth()) / 2;
            _image->draw(_parent.getWindow(),
                         x, _gm.y,
                         _image->getWidth(), _image->getHeight());
        }

    private:
        PImage *_image;
    };

    class Text : public Widget {
    public:
        Text(Theme::DialogData& data, PWinObj& parent,
             const std::wstring& text, bool is_title)
            : Widget(data, parent),
              _font(is_title ? data.getTitleFont() : data.getTextFont()),
              _text(text),
              _is_title(is_title)
        {
        }
        virtual ~Text(void) { }

        virtual uint heightReq(uint width) const override
        {
            // TODO: split up in lines
            return _font->getHeight() + _data.padVert();
        }

        virtual void render(void) override
        {
            _font->setColor(_is_title
                            ? _data.getTitleColor() : _data.getTextColor());
            _font->draw(_parent.getWindow(),
                        _gm.x + _data.getPad(PAD_LEFT),
                        _gm.y + _data.getPad(PAD_UP), _text);
        }

    private:
        PFont *_font;
        std::wstring _text;
        bool _is_title;
    };

    PekwmDialog(Theme::DialogData& data,
                const Geometry &gm,
                const std::wstring& title, PImage* image,
                const std::wstring& message,
                std::vector<std::wstring> options)
        : PWinObj(true),
          _data(data)
    {
        // TODO: setup size minimum based on image
        initWindow(title);
        initWidgets(title, image, message, options);
        placeWidgets();
    }

    ~PekwmDialog(void)
    {
        for (auto it : _widgets) {
            delete it;
        }
        X11::destroyWindow(_window);
    }

    virtual void resize(uint width, uint height) override
    {
        PWinObj::resize(width, height);
        for (auto it : _widgets) {
            it->place(it->getX(), it->getY(), width);
        }
        render();
    }

    void show(void)
    {
        X11::mapWindow(_window);
        render();
    }

    void render(void)
    {
        X11::clearWindow(_window);
        _data.getBackground()->render(_window, 0, 0, _gm.width, _gm.height);
        for (auto it : _widgets) {
            it->render();
        }
    }

    void setState(Window window, ButtonState state)
    {
        for (auto it : _widgets) {
            if (it->setState(window, state)) {
                break;
            }
        }
    }

    void click(Window window)
    {
        for (auto it : _widgets) {
            if (it->click(window)) {
                break;
            }
        }
    }

protected:
    void initWindow(const std::wstring& title)
    {
        _window =
            X11::createSimpleWindow(X11::getRoot(),
                                    _gm.x, _gm.y, _gm.width, _gm.height, 0,
                                    X11::getBlackPixel(), X11::getWhitePixel());
        X11::selectInput(_window,
                         ExposureMask|StructureNotifyMask);

        auto c_title = new uchar[title.size() + 1];
        memcpy(c_title, title.c_str(), title.size() + 1);

        XSizeHints normal_hints = {0};
        XWMHints wm_hints = {0};
        wm_hints.flags = StateHint|InputHint;
        wm_hints.initial_state = NormalState;
        wm_hints.input = True;

        char wm_name[] = "dialog";
        char wm_class[] = "pekwm_dialog";
        XClassHint class_hint = {wm_name, wm_class};

        auto title_utf8 =
            Util::to_utf8_str(title.size() ? title : L"pekwm_dialog");
        Xutf8SetWMProperties(X11::getDpy(), _window,
                             title_utf8.c_str(), title_utf8.c_str(), 0, 0,
                             &normal_hints, &wm_hints, &class_hint);

        delete [] c_title;

        X11::setAtom(_window, WINDOW_TYPE, WINDOW_TYPE_NORMAL);
    }

    void initWidgets(const std::wstring& title, PImage* image,
                     const std::wstring& message,
                     std::vector<std::wstring> options)
    {
        if (title.size()) {
            _widgets.push_back(new PekwmDialog::Text(_data, *this, title, true));
        }
        if (image) {
            _widgets.push_back(new PekwmDialog::Image(_data, *this, image));
        }
        if (message.size()) {
            _widgets.push_back(new PekwmDialog::Text(_data, *this, message, false));
        }
        _widgets.push_back(new PekwmDialog::ButtonsRow(_data, *this, options));
    }

    void placeWidgets(void)
    {
        // height is dependent on the available width, get requested
        // width first.
        uint width = _gm.width;
        for (auto it : _widgets) {
            uint width_req = it->widthReq();
            if (width_req && width_req > width) {
                width = width_req;
            }
        }
        if (width == 0) {
            width = WIDTH_DEFAULT;
        }

        uint y = 0;
        for (auto it : _widgets) {
            it->place(0, y, width);
            y += it->heightReq(width);
        }

        PWinObj::resize(std::max(width, _gm.width), std::max(y, _gm.height));
    }

private:
    Theme::DialogData& _data;
    std::vector<PekwmDialog::Widget*> _widgets;
};

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
    std::cout << "usage: " << name
              << " [-dhit] [-o option|-o option...] message" << std::endl
              << "  -d --display dpy    Display" << std::endl
              << "  -h --help           Display this information" << std::endl
              << "  -i --image          Image under title" << std::endl
              << "  -o --option         Option (many allowed)" << std::endl
              << "  -t --title          Dialog title" << std::endl;
    exit(ret);
}

static int mainLoop(PekwmDialog& dialog)
{
    XEvent ev;
    while (_stop == -1 && X11::getNextEvent(ev)) {
        switch (ev.type) {
        case ButtonPress:
            dialog.setState(ev.xbutton.window, BUTTON_STATE_PRESSED);
            break;
        case ButtonRelease:
            dialog.click(ev.xbutton.window);
            break;
        case ConfigureNotify:
            dialog.resize(ev.xconfigure.width, ev.xconfigure.height);
            break;
        case EnterNotify:
            dialog.setState(ev.xbutton.window, BUTTON_STATE_HOVER);
            break;
        case Expose:
            dialog.render();
            break;
        case LeaveNotify:
            dialog.setState(ev.xbutton.window, BUTTON_STATE_FOCUSED);
            break;
        case MapNotify:
            break;
        case ReparentNotify:
            break;
        case UnmapNotify:
            break;
        default:
            DBG("UNKNOWN EVENT " << ev.type);
            break;
        }
    }
    return _stop;
}

static void getThemeDir(std::string& dir, std::string& variant)
{
    std::string home_config("~/.pekwm/config");
    Util::expandFileName(home_config);

    CfgParser cfg;
    cfg.parse(home_config, CfgParserSource::SOURCE_FILE, true);
    auto files = cfg.getEntryRoot()->findSection("FILES");
    if (files != nullptr) {
        std::vector<CfgParserKey*> keys;
        keys.push_back(new CfgParserKeyPath("THEME", dir, THEME_DEFAULT));
        keys.push_back(new CfgParserKeyString("THEMEVARIANT", variant));
        files->parseKeyValues(keys.begin(), keys.end());
        std::for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
    } else {
        dir = THEME_DEFAULT;
        variant = "";
    }
}

int main(int argc, char* argv[])
{
    const char* display = NULL;
    Geometry gm = { 0, 0, WIDTH_DEFAULT, HEIGHT_DEFAULT };
    int gm_mask = WIDTH_VALUE | HEIGHT_VALUE;
    std::wstring title;
    std::string image_name;
    std::vector<std::wstring> options;

    static struct option opts[] = {
        {"display", required_argument, NULL, 'd'},
        {"geometry", required_argument, NULL, 'g'},
        {"help", no_argument, NULL, 'h'},
        {"image", required_argument, NULL, 'i'},
        {"option", required_argument, NULL, 'o'},
        {"title", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}
    };

    try {
        std::locale::global(std::locale(""));
    } catch (const std::runtime_error &e) {
        setlocale(LC_ALL, "");
    }

    Util::iconv_init();

    int ch;
    while ((ch = getopt_long(argc, argv, "d:g:hi:o:t:", opts, NULL)) != -1) {
        switch (ch) {
        case 'd':
            display = optarg;
            break;
        case 'g':
            gm_mask |= X11::parseGeometry(optarg, gm);
            break;
        case 'h':
            usage(argv[0], 0);
            break;
        case 'i':
            image_name = optarg;
            Util::expandFileName(image_name);
            break;
        case 'o':
            options.push_back(Util::to_wide_str(optarg));
            break;
        case 't':
            title = Util::to_wide_str(optarg);
            break;
        default:
            usage(argv[0], 1);
            break;
        }
    }

    if (options.empty()) {
        options.push_back(L"Ok");
    }

    std::wstring message;
    for (int i = optind; i < argc; i++) {
        if (i > optind) {
            message += L' ';
        }
        message += Util::to_wide_str(argv[i]);
    }

    auto dpy = XOpenDisplay(display);
    if (! dpy) {
        std::cerr << "Can not open display!" << std::endl
                  << "Your DISPLAY variable currently is set to: "
                  << Util::getEnv("DISPLAY") << std::endl;
        return 1;
    }

    X11::init(dpy, true);
    init(dpy);

    _image_handler->path_push_back("./");
    PImage *image = nullptr;
    if (image_name.size()) {
        image = _image_handler->getImage(image_name);
    }

    std::string theme_dir, theme_variant;
    getThemeDir(theme_dir, theme_variant);

    int ret;
    {
        // run in separate scope to get Theme destructed before cleanup
        Theme theme(_font_handler, _image_handler, _texture_handler,
                    theme_dir, theme_variant);
        PekwmDialog dialog(theme.getDialogData(),
                           gm, title, image, message, options);
        dialog.show();

        ret = mainLoop(dialog);
    }
    if (image) {
        _image_handler->returnImage(image);
    }

    cleanup();
    X11::destruct();
    Util::iconv_deinit();

    return ret;
}
