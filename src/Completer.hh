//
// Completer.cc for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _COMPLETER_HH_
#define _COMPLETER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <list>
#include <string>

typedef std::list<std::wstring> complete_list;
typedef complete_list::iterator complete_it;

/**
 * Base class for completer methods, provides method to see if it
 * should be used and also actual completion.
 */
class CompleterMethod
{
public:
    /** Constructor for CompleterMethod, refresh completion list. */
    CompleterMethod(void) { }
    /** Destructor for CompleterMethod */
    virtual ~CompleterMethod(void) { }

    /** Return true if method can complete. */
    virtual bool can_complete(const std::wstring &str) { return false; }
    /** Find completions for string. */
    virtual unsigned int complete(const std::wstring &str, const std::wstring &word,
                                  complete_list &completions) { return 0; }
    /** Refresh completion list. */
    virtual void refresh(void) { }
};

/**
 * Null completer, provides no completion independent of input.
 */
class NullCompleterMethod : public CompleterMethod
{
public:
    /** Null completer can always complete. */
    virtual bool can_complete(const std::wstring &str) { return true; }
    /** Find completions, does nothing. */
    virtual unsigned int complete(const std::wstring &str, const std::wstring &word,
                                  complete_list &completions) { return 0; }
};

/**
 * Path completer, provides completion of elements in the path.
 */
class PathCompleterMethod : public CompleterMethod
{
public:
    /** Constructor for PathCompleter method. */
    PathCompleterMethod(void) : CompleterMethod() { refresh(); }
    /** Destructor for PathCompleterMethod */
    virtual ~PathCompleterMethod(void) { }

    /** Path completer can always complete. */
    virtual bool can_complete(const std::wstring &str) { return true; }
    virtual unsigned int complete(const std::wstring &str, const std::wstring &word,
                                  complete_list &completions);
    virtual void refresh(void);

private:
    std::list<std::wstring> _path_list; /**< List of all elements in path. */
};

/**
 * Action completer, provides completion of all available actions in
 * pekwm.
 */
class ActionCompleterMethod : public CompleterMethod
{
public:
    /** Path completer can always complete. */
    virtual bool can_complete(const std::wstring &str) { return true; }
    virtual unsigned int complete(const std::wstring &str, const std::wstring &word,
                                  complete_list &completions);
    virtual void refresh(void);

private:
    std::list<std::wstring> _action_list; /**< List of all available actions. */
    std::list<std::wstring> _state_param_list; /**< List of parameters to state actions. */
};

/**
 * Completer class, has a set of completer methods which provides
 * completions. Handles the string handling to detect the action,
 * replacing completion results etc.
 */
class Completer
{
public:
    /** Completer constructor */
    Completer(const std::wstring separators = L"") : _separators(separators) { }
    /** Completer destructor. */
    ~Completer(void);

    /** Add method to completer. */
    void add_method(CompleterMethod *method) { _methods.push_back(method); }

    complete_list find_completions(const std::wstring &str, unsigned int pos);
    std::wstring do_complete(const std::wstring &str, unsigned int &pos,
                             complete_list &completions, complete_it &it);

private:
    std::wstring get_part(const std::wstring &str, unsigned int pos,
                          size_t &part_begin, size_t &part_end);
    std::wstring get_word_at_position(const std::wstring &str, unsigned int pos,
                                      size_t &word_begin, size_t &word_end);

private:
    std::list<CompleterMethod*> _methods; /**< List of CompleterMethods. */
    const std::wstring _separators; /**< String with separator characters. */
};

#endif // _COMPLETER_HH_
