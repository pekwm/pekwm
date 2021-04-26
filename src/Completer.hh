//
// Completer.cc for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_COMPLETER_HH_
#define _PEKWM_COMPLETER_HH_

#include "config.h"

#include <vector>
#include <string>

class CompPair
{
public:
    CompPair(const std::string& first,
             const std::string& second);
    ~CompPair(void);

    bool operator==(const CompPair& rhs) const;
    bool operator<(const CompPair& rhs) const;

public:
    std::string first;
    std::string second;
};

typedef std::vector<std::string> complete_list;
typedef complete_list::iterator complete_it;
typedef std::vector<CompPair> completions_list;
typedef completions_list::iterator completions_it;

/**
 * State data used during completion.
 */
class CompletionState {
public:
    std::string part;
    std::string part_lower;
    size_t part_begin, part_end;
    std::string word;
    std::string word_lower;
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

    void refresh();
    void clear();

    complete_list find_completions(const std::string &str, unsigned int pos);
    std::string do_complete(const std::string &str, unsigned int &pos,
                             complete_list &completions, complete_it &it);

private:
    std::string get_part(const std::string &str, unsigned int pos,
                          size_t &part_begin, size_t &part_end);
    std::string get_word_at_position(const std::string &str, unsigned int pos,
                                      size_t &word_begin, size_t &word_end);

    CompleterMethod *_completer_action;
    CompleterMethod *_completer_path;
};

#endif // _PEKWM_COMPLETER_HH_
