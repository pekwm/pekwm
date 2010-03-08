//
// Completer.cc for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <list>
#include <vector>
#include <string>

extern "C" {
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
}

#include "Completer.hh"
#include "Config.hh"
#include "Util.hh"
#ifdef MENUS
#include "PWinObj.hh"
#include "MenuHandler.hh"
#endif // MENUS

using std::copy;
using std::cerr;
using std::wcerr;
using std::endl;
using std::list;
using std::vector;
using std::string;
using std::wstring;
using std::pair;

ActionCompleterMethod::StateMatch ActionCompleterMethod::STATE_MATCHES[] = {
  StateMatch(ActionCompleterMethod::STATE_STATE, L"set"),
  StateMatch(ActionCompleterMethod::STATE_STATE, L"unset"),
  StateMatch(ActionCompleterMethod::STATE_STATE, L"toggle"),
  StateMatch(ActionCompleterMethod::STATE_MENU, L"showmenu")
  };

/**
 * Find matches for word in completions_list and add to completions.
 */
unsigned int
CompleterMethod::complete_word(completions_list &completions_list,
                               complete_list &completions,
                               const std::wstring &word)
{
    unsigned int completed = 0, equality = -1;

    completions_it it(completions_list.begin());
    for (; it != completions_list.end(); ++it) {
        if (it->first.size() < word.size()) {
            continue;
        }
        equality = it->first.compare(0, word.size(), word, 0, word.size());
        if (equality == 0) {
            completions.push_back(it->second);
            completed++;
        }
    }

    return completed;
}

/**
 * Complete str with available path elements.
 */
unsigned int
PathCompleterMethod::complete(CompletionState &completion_state)
{
    return complete_word(_path_list,
                         completion_state.completions,
                         completion_state.word);
}

/**
 * Refresh available completions.
 */
void
PathCompleterMethod::refresh(void)
{
    // Clear out previous data
    _path_list.clear();

    vector<string> path_parts;
    Util::splitString(getenv("PATH") ? getenv("PATH") : "", path_parts, ":");

    vector<string>::iterator it(path_parts.begin());
    for (; it != path_parts.end(); ++it) {
        DIR *dh = opendir(it->c_str());
        if (dh) {
            refresh_path(dh, Util::to_wide_str(*it));
            closedir(dh);
        }
    }

    _path_list.unique();
    _path_list.sort();
}

/**
 * Refresh single directory.
 */
void
PathCompleterMethod::refresh_path(DIR *dh, const std::wstring path)
{
    struct dirent *entry;
    while ((entry = readdir(dh)) != 0) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        wstring name(Util::to_wide_str(entry->d_name));
        _path_list.push_back(pair<wstring, wstring>(name, name));
        _path_list.push_back(pair<wstring, wstring>(path + L"/" + name,
                                                    path + L"/" + name));
    }
}

/**
 * Check if str matches state prefix.
 */
bool
ActionCompleterMethod::StateMatch::is_state(const wstring &str, size_t pos)
{
    if (str.size() - pos < _prefix_len) {
        return false;
    } else {
        return str.compare(pos, _prefix_len, _prefix, _prefix_len) == 0;
    }
}

/**
 * Complete action.
 */
unsigned int
ActionCompleterMethod::complete(CompletionState &completion_state)
{
    State state = find_state(completion_state);
    switch (state) {
    case STATE_STATE:
        return complete_state(completion_state.word_lower,
                              completion_state.completions);
#ifdef MENUS
    case STATE_MENU:
        return complete_menu(completion_state.word_lower,
                             completion_state.completions);
#endif // MENUS
    case STATE_ACTION:
    default:
        return complete_action(completion_state.word_lower,
                               completion_state.completions);
    }
}

/**
 * Complete state actions.
 */
unsigned int
ActionCompleterMethod::complete_state(const wstring &word,
                                      complete_list &completions)
{
    return complete_word(_state_list, completions, word);
}

#ifdef MENUS
/**
 * Complete menu names.
 */
unsigned int
ActionCompleterMethod::complete_menu(const wstring &word,
                                     complete_list &completions)
{
    return complete_word(_menu_list, completions, word);
}
#endif // MENUS

/**
 * Complete action names.
 */
unsigned int
ActionCompleterMethod::complete_action(const wstring &word,
                                       complete_list &completions)
{
    return complete_word(_action_list, completions, word);
}

/**
 * Build list of completions from available actions.
 */
void
ActionCompleterMethod::refresh(void)
{
    // Grab list of actions
    _action_list.clear();
    Config::action_map_it action_it(Config::instance()->action_map_begin()), action_it_end(Config::instance()->action_map_end());
    for (; action_it != action_it_end; ++action_it) {
        if (action_it->second.second&KEYGRABBER_OK) {
            wstring action_name(Util::to_wide_str(action_it->first.get_text()));
            wstring action_name_lower(action_name);
            Util::to_lower(action_name_lower);
            pair<wstring, wstring> action_pair(action_name_lower, action_name);
            _action_list.push_back(action_pair);
        }
    }
    _action_list.sort();

    // Grab list of state actions
    _state_list.clear();
    Config::action_state_map_it state_it(Config::instance()->action_state_map_begin()), state_it_end(Config::instance()->action_state_map_end());
    for (; state_it != state_it_end; ++state_it) {
        wstring state_name(Util::to_wide_str(state_it->first.get_text()));
        wstring state_name_lower(state_name);
        Util::to_lower(state_name_lower);
        pair<wstring, wstring> state_pair(state_name_lower, state_name);
        _state_list.push_back(state_pair);
    }
    _state_list.sort();

#ifdef MENUS
    _menu_list.clear();
    list<string> menu_names(MenuHandler::instance()->getMenuNames());
    list<string>::iterator menu_it(menu_names.begin());
    for (; menu_it != menu_names.end(); ++menu_it) {
        wstring menu_name(Util::to_wide_str(*menu_it));
        wstring menu_name_lower(menu_name);
        Util::to_lower(menu_name_lower);
        pair<wstring, wstring> menu_pair(menu_name_lower, menu_name);
        _menu_list.push_back(menu_pair);
    }
    _menu_list.unique();
    _menu_list.sort();
#endif // MENUS
}

/**
 * Detect state being completed
 */
ActionCompleterMethod::State
ActionCompleterMethod::find_state(CompletionState &completion_state)
{
    State state = STATE_NO;
    if (completion_state.word_begin != 0) {
        state = find_state_match(completion_state.part_lower,
                                 completion_state.part_begin);
    }
    return state;
}

/**
 * Find matching state.
 */
ActionCompleterMethod::State
ActionCompleterMethod::find_state_match(const std::wstring &str, size_t pos)
{
    for (int i = 0; i < STATE_NUM; ++i) {
        if (STATE_MATCHES[i].is_state(str, pos)) {
            return STATE_MATCHES[i].get_state();
        }
    }
    return STATE_NO;
}

/**
 * Completer destructor, free up resources used by methods.
 */
Completer::~Completer(void)
{
    list<CompleterMethod*>::iterator it(_methods.begin());
    for (; it != _methods.end(); ++it) {
        delete *it;
    }
}

/**
 * Find completions for string with the cursor at position.
 */
complete_list
Completer::find_completions(const wstring &str, unsigned int pos)
{
    // Get current part of str, if it is empty return no completions.
    CompletionState state;
    state.part = state.part_lower = get_part(str, pos, state.part_begin, state.part_end);
    if (! state.part.size()) {
        return state.completions;
    }

    // Get word at position, the one that will be completed
    state.word = state.word_lower = get_word_at_position(str, pos, state.word_begin, state.word_end);

    Util::to_lower(state.part_lower);
    Util::to_lower(state.word_lower);

    // Go through completer methods and add completions.
    list<CompleterMethod*>::iterator it(_methods.begin());
    for (; it != _methods.end(); ++it) {
        if ((*it)->can_complete(state.part)) {
            (*it)->complete(state);
        }
    }

    return state.completions;
}

/**
 * Perform actual completion, returns new string with completion
 * inserted if any.
 *
 * @param str String to complete.
 * @param pos Cursor position in string.
 * @return Completed string.
 */
wstring
Completer::do_complete(const wstring &str, unsigned int &pos,
                       complete_list &completions, complete_it &it)
{
    // Do not perform completion if there is nothing to complete
    if (! completions.size()) {
        return str;
    }
    // Wrap completions, return original string
    if (it == completions.end()) {
        it = completions.begin();
        return str;
    }

    // Get current word, this is the one being replaced
    size_t word_begin, word_end;
    wstring word(get_word_at_position(str, pos, word_begin, word_end));

    // Replace the current word
    wstring completed(str);
    completed.replace(word_begin, word_end - word_begin, *it);

    // Update position
    pos = word_begin + it->size();

    // Increment completion
    it++;

    return completed;
}

/**
 * Find current part being completed, string can be split up with
 * separators and only one part should be treated at a time.
 *
 * @param str String to find part in.
 * @param pos Position in string.
 * @param part_begin
 * @param part_end
 * @return Current part of string.
 */
wstring
Completer::get_part(const wstring &str, unsigned int pos, size_t &part_begin, size_t &part_end)
{
    // If no separators are defined, do nothing.
    if (! _separators.size()) {
        return str;
    }

    // Get beginning and end of string, add 1 for removal of separator
    part_begin = String::safe_position(str.find_last_of(_separators, pos), 0, 1);
    part_end = String::safe_position(str.find_first_of(_separators, pos), str.size());

    // Strip spaces from the beginning of the string
    part_begin = String::safe_position(str.find_first_not_of(L" \t", part_begin), part_end);

    return str.substr(part_begin, part_end - part_begin);
}

/**
 * Get word at position.
 */
wstring
Completer::get_word_at_position(const wstring &str, unsigned int pos, size_t &word_begin, size_t &word_end)
{
    // Get beginning and end of string, add 1 for removal of separator
    word_begin = String::safe_position(str.find_last_of(L" \t", pos), 0, 1);
    word_end = String::safe_position(str.find_first_of(L" \t", pos), str.size());

    return str.substr(word_begin, word_end - word_begin);    
}
