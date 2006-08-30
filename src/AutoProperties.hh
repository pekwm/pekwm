//
// AutoProperties.hh for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _AUTOPROPERTIES_HH_
#define _AUTOPROPERTIES_HH_

#include "pekwm.hh"
#include "CfgParser.hh"
#include "RegexString.hh"
#include "ParseUtil.hh"

#include <string>
#include <list>

class RegexString;

enum PropertyType {
    AP_STICKY = (1L << 1),
    AP_SHADED = (1L << 2),
    AP_MAXIMIZED_VERTICAL = (1L << 3),
    AP_MAXIMIZED_HORIZONTAL = (1L << 4),
    AP_ICONIFIED = (1L << 5),
    AP_BORDER = (1L << 6),
    AP_TITLEBAR = (1L << 7),
    AP_FRAME_GEOMETRY = (1L << 8),
    AP_CLIENT_GEOMETRY = (1L << 9),
    AP_LAYER = (1L << 10),
    AP_WORKSPACE = (1L << 11),
    AP_SKIP = (1L << 12),
    AP_FULLSCREEN = (1L << 13),
    AP_VIEWPORT = (1L << 14),
    AP_PLACE_NEW = (1L << 15),
    AP_FOCUS_NEW = (1L << 16),
    AP_FOCUSABLE = (1L << 17),
    AP_CFG_DENY = (1L << 18),

    AP_GROUP_SIZE,
    AP_GROUP_BEHIND,
    AP_GROUP_FOCUSED_FIRST,
    AP_GROUP_GLOBAL,
    AP_GROUP_RAISE,

    AP_PROPERTY,
    AP_NO_PROPERTY
};

class ClassHint {
public:
    ClassHint(void) { }
    ~ClassHint(void) { }

    inline ClassHint& operator = (const ClassHint& rhs) {
        h_name = rhs.h_name;
        h_class = rhs.h_class;
        h_role = rhs.h_role;
        title = rhs.title;
        group = rhs.group;

        return *this;
    }
    inline bool operator == (const ClassHint& rhs) const {
        if (group.size() > 0) {
            if (group == rhs.group)
                return true;
        } else if ((h_name == rhs.h_name) && (h_class == rhs.h_class) &&
                   (h_role == rhs.h_role)) {
            return true;
        }

        return false;
    }

public:
    std::string h_name, h_class, h_role;
    std::string title, group;
};

class Property {
public:
    Property(void) : _apply_mask(0) { }
    virtual ~Property(void) { }

    inline RegexString& getHintName(void) { return _hint_name; }
    inline RegexString& getHintClass(void) { return _hint_class; }
    inline RegexString& getRole(void) { return _role; }
    inline RegexString& getTitle(void) { return _title; }

    inline uint getApplyOn(void) const { return _apply_mask; }

    inline bool isApplyOn(uint mask) const { return (_apply_mask&mask); }
    inline void applyAdd(uint mask) { _apply_mask |= mask; }
    inline void applyRemove(uint mask) { _apply_mask &= ~mask; }

    inline std::list<uint> getWsList(void) { return _ws_list; }

private:
    RegexString _hint_name, _hint_class;
    RegexString _role, _title;

    uint _apply_mask;
    std::list<uint> _ws_list;
};

// AutoProperty for everything except title rewriting
class AutoProperty : public Property
{
public:
    AutoProperty(void) : cfg_deny(0),
            group_size(-1), group_behind(false), group_focused_first(false),
    group_global(false), group_raise(false), _prop_mask(0) { }
    virtual ~AutoProperty(void) { }

    inline bool isMask(uint mask) { return (_prop_mask&mask); }
    inline void maskAdd(uint mask) { _prop_mask |= mask; }
    inline void maskRemove(uint mask) { _prop_mask &= ~mask; }

public:
    Geometry frame_gm, client_gm;
    int frame_gm_mask, client_gm_mask;

    bool sticky, shaded, iconified;
    bool maximized_vertical, maximized_horizontal, fullscreen;
    bool border, titlebar;
    bool focusable, place_new, focus_new;
    uint workspace, layer, skip, viewport_col, viewport_row, cfg_deny;

    std::string frame_decor;

    // grouping variables
    int group_size;
    std::string group_name;
    bool group_behind, group_focused_first, group_global, group_raise;

private:
    uint _prop_mask;
};

// TitleProperty for title rewriting
class TitleProperty : public Property
{
public:
    TitleProperty(void) { }
    virtual ~TitleProperty(void) { }

    RegexString& getTitleRule(void) { return _title_rule; }

private:
    RegexString _title_rule;
};

// DecorProperty for multiple decor types
class DecorProperty : public Property
{
public:
    DecorProperty(void) { }
    virtual ~DecorProperty(void) { }

    inline const std::string &getName(void) const { return _decor_name; }
    inline void setName(const std::string &name) { _decor_name = name; }

private:
    std::string _decor_name;
};

#ifdef HARBOUR
// DockApp for Harbour sorting
class DockAppProperty : public Property
{
public:
    DockAppProperty(void) : _position(0) { }
    virtual ~DockAppProperty(void) { }

    inline int getPosition(void) const { return _position; }
    inline void setPosition(int position) { _position = position; }

private:
    int _position;
};
#endif // HARBOUR

class AutoProperties {
public:
    AutoProperties(void);
    ~AutoProperties(void);

    static inline AutoProperties *instance(void) { return _instance; }

    AutoProperty* findAutoProperty(const ClassHint* class_hintbb,
                                   int ws = -1, uint type = 0);
    TitleProperty* findTitleProperty(const ClassHint* class_hint);
    DecorProperty* findDecorProperty(const ClassHint* class_hint);
#ifdef HARBOUR
    DockAppProperty* findDockAppProperty(const ClassHint *class_hint);
    inline bool isHarbourSort(void) const { return _harbour_sort; }
#endif // HARBOUR

    void load(void);
    void unload(void);

    void removeApplyOnStart(void);

    static bool matchAutoClass(const ClassHint &hint, Property *prop);

private:
    Property* findProperty(const ClassHint* class_hint,
                           std::list<Property*>* prop_list,
                           int ws, uint type);

    bool parseProperty(CfgParser::Entry *op_section, Property *prop);
    void parseAutoProperty(CfgParser::Entry *op_section, std::list<uint>* ws);
    void parseAutoGroup(CfgParser::Entry *op_section, AutoProperty* prop);
    void parseTitleProperty(CfgParser::Entry *op_section);
    void parseDecorProperty(CfgParser::Entry *op_section);
#ifdef HARBOUR
    void parseDockAppProperty(CfgParser::Entry *op_section);
#endif // HARBOUR

private:
    std::list<Property*> _prop_list;
    std::list<Property*> _title_prop_list;
    std::list<Property*> _decor_prop_list;
#ifdef HARBOUR
    std::list<Property*> _dock_app_prop_list;
    bool _harbour_sort;
#endif // HARBOUR
    bool _apply_on_start;

    std::map<ParseUtil::Entry, ApplyOn> _apply_on_map;
    std::map<ParseUtil::Entry, PropertyType> _property_map;
    std::map<ParseUtil::Entry, PropertyType> _group_property_map;

    static AutoProperties *_instance;
};

#endif // _AUTOPROPS_HH_
