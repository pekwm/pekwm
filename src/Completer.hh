//
// Completer.cc for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
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

extern "C" {
#include <sys/types.h>
#include <dirent.h>
}

typedef std::list<std::wstring> complete_list;
typedef complete_list::iterator complete_it;
typedef std::list<std::pair<std::wstring, std::wstring> > completions_list;
typedef completions_list::iterator completions_it;

/**
 * State data used during completion.
 */
class CompletionState {
public:
    std::wstring part;
    std::wstring part_lower;
    size_t part_begin, part_end;
    std::wstring word;
    std::wstring word_lower;
    size_t word_begin, word_end;
    complete_list completions;
};

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
    virtual unsigned int complete(CompletionState &completion_state) { return 0; }
    /** Refresh completion list. */
    virtual void refresh(void) { }

protected:
    unsigned int complete_word(completions_list &completions_list,
                               complete_list &completions,
                               const std::wstring &word);
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
    virtual unsigned int complete(CompletionState &completion_state) { return 0; }
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
    virtual unsigned int complete(CompletionState &completion_state);
    virtual void refresh(void);

private:
    void refresh_path(DIR *dh, const std::wstring path);

private:
    completions_list _path_list; /**< List of all elements in path. */
};

/**
 * Action completer, provides completion of all available actions in
 * pekwm.
 */
class ActionCompleterMethod : public CompleterMethod
{
public:
    /**
     * States for context sensitive ActionCompleterMethod completions.
     */
    enum State {
        STATE_ACTION,
        STATE_STATE,
        STATE_MENU,
        STATE_NO,
        STATE_NUM = 5
    };

    /**
     * Context match information.
     */
    class StateMatch {
    public:
        StateMatch(State state, const wchar_t *prefix)
           : _prefix(prefix), _prefix_len(wcslen(prefix)), _state(state) {
        }

        State get_state(void) { return _state; }
        bool is_state(const std::wstring &str, size_t pos);
    private:
        const wchar_t *_prefix; /**< Matching prefix */
        const size_t _prefix_len; /**< */
        State _state; /**< State */
    };

    /** Constructor for ActionCompleter method. */
    ActionCompleterMethod(void) : CompleterMethod() { refresh(); }
    /** Destructor for ActionCompleterMethod */
    virtual ~ActionCompleterMethod(void) { }

    /** Path completer can always complete. */
    virtual bool can_complete(const std::wstring &str) { return true; }
    virtual unsigned int complete(CompletionState &completion_state);
    virtual void refresh(void);

private:
    State find_state(CompletionState &completion_state);
    size_t find_state_word_start(const std::wstring &str);
    State find_state_match(const std::wstring &str, size_t pos);

private:
    completions_list _action_list; /**< List of all available actions. */
    completions_list _state_list; /**< List of parameters to state actions. */
    completions_list _menu_list; /**< List of parameters to state actions. */
    static StateMatch STATE_MATCHES[]; /**< List of known states with matching data. */
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
