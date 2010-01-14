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

using std::cerr;
using std::wcerr;
using std::endl;
using std::list;
using std::vector;
using std::string;
using std::wstring;

/**
 * Complete str with available path elements.
 */
unsigned int
PathCompleterMethod::complete(const wstring &str, const wstring &word, complete_list &completions)
{
    unsigned int completed = 0;

    list<wstring>::iterator it(_path_list.begin());
    for (; it != _path_list.end(); ++it) {
        // FIXME: Break when comparsion is > 0
        if (it->size() >= word.size()
            && it->compare(0, word.size(), word, 0, word.size()) == 0) {
            completions.push_back(*it);
            completed++;
        }
    }

    return completed;
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
        if (! dh) {
            continue;
        }

        wstring path(Util::to_wide_str(*it));
        struct dirent *entry;
        while ((entry = readdir(dh)) != 0) {
            if (entry->d_name[0] == '.') {
                continue;
            }
            
            wstring name(Util::to_wide_str(entry->d_name));
            _path_list.push_back(name);
            _path_list.push_back(path + L"/" + name);
        }

        closedir(dh);
    }

    _path_list.sort();
}

/**
 * Build list of completions from available actions.
 */
void
ActionCompleterMethod::refresh(void)
{
    // Grab list of actions
    Config::action_map_it action_it(Config::instance()->action_map_begin()), action_it_end(Config::instance()->action_map_end());
    for (; action_it != action_it_end; ++action_it) {
        if (action_it->second.second&KEYGRABBER_OK) {
            _action_list.push_back(Util::to_wide_str(action_it->first.get_text()));
        }
    }
    _action_list.sort();
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
    complete_list completions;

    // Get current part of str, if it is empty return no completions.
    size_t part_begin, part_end;
    wstring part(get_part(str, pos, part_begin, part_end));
    if (! part.size()) {
        return completions;
    }

    // Get word at position, the one that will be completed
    size_t word_begin, word_end;
    wstring word(get_word_at_position(str, pos, word_begin, word_end));    

    // Go through completer methods and add completions.
    list<CompleterMethod*>::iterator it(_methods.begin());
    for (; it != _methods.end(); ++it) {
        if ((*it)->can_complete(part)) {
            (*it)->complete(part, word, completions);
        }
    }

    return completions;
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
Completer::do_complete(const wstring &str, unsigned int &pos, complete_list &completions, complete_it &it)
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
