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

#include <vector>
#include <string>

typedef std::vector<std::wstring> complete_list;
typedef complete_list::iterator complete_it;
typedef std::vector<std::pair<std::wstring, std::wstring> > completions_list;
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

class CompleterMethod;

/**
 * Completer class, has a set of completer methods which provides
 * completions. Handles the string handling to detect the action,
 * replacing completion results etc.
 */
class Completer
{
public:
    /** Completer constructor */
    Completer(void);
    /** Completer destructor. */
    ~Completer(void);

    complete_list find_completions(const std::wstring &str, unsigned int pos);
    std::wstring do_complete(const std::wstring &str, unsigned int &pos,
                             complete_list &completions, complete_it &it);

private:
    std::wstring get_part(const std::wstring &str, unsigned int pos,
                          size_t &part_begin, size_t &part_end);
    std::wstring get_word_at_position(const std::wstring &str, unsigned int pos,
                                      size_t &word_begin, size_t &word_end);

    CompleterMethod *_completer_action;
    CompleterMethod *_completer_path;
};

#endif // _COMPLETER_HH_
