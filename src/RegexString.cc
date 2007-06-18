//
// RegexString.cc for pekwm
// Copyright ©  2003-2007 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>

#include "RegexString.hh"
#include "Util.hh"

#ifdef HAVE_GETTEXT
#include <libintl.h>
#define _(S) gettext(S)
#else // !HAVE_GETTEXT
#define _(S) S
#endif // HAVE_GETTEXT

using std::cerr;
using std::endl;
using std::list;
using std::string;
using std::wstring;

//! @brief RegexString constructor.
RegexString::RegexString (void)
    : m_reg_ok(false), m_i_ref_max(1)
{
}

//! @brief RegexString destructor.
RegexString::~RegexString (void)
{
    free_regex();
}

//! @brief Simple ed s command lookalike.
bool
RegexString::ed_s (std::wstring &or_string)
{
    if (!m_reg_ok) {
        return false;
    }

    string str(Util::to_mb_str(or_string));

    const char *op_str = str.c_str();
    regmatch_t *op_matches = new regmatch_t[m_i_ref_max];

    // Match
    if (regexec(&m_o_regex, str.c_str(), m_i_ref_max, op_matches, 0)) {
        delete [] op_matches;
        return false;
    }

    string o_result;
    uint i_ref, i_size;

    list<RegexString::Part>::iterator it(m_o_ref_list.begin ());
    for (; it != m_o_ref_list.end(); ++it) {
        if (it->get_reference() >= 0) {
            i_ref = it->get_reference();

            if (op_matches[i_ref].rm_so != -1) {
                i_size = op_matches[i_ref].rm_eo - op_matches[i_ref].rm_so;
                o_result.append(string(op_str + op_matches[i_ref].rm_so,
                                       i_size));
            }
        } else {
            o_result.append(Util::to_mb_str(it->get_string()));
        }
    }

    // Replace area regexp matched.
    i_size = op_matches[0].rm_eo - op_matches[0].rm_so;
    str.replace(op_matches[0].rm_so, i_size, o_result);

    or_string = Util::to_wide_str(str);

    return true;
}

//! @brief Parses match part of regular expression.
//! @param or_match Expression.
//! @param full Full expression if true (including flags). Defaults to false.
bool
RegexString::parse_match(const std::wstring &or_match, bool full)
{
    // Free resources
    if (m_reg_ok) {
        free_regex();
    }

    if (or_match.size()) {
        int flags = REG_EXTENDED;
        string expression;
        wstring expression_str;

        if (full) {
            // Full regular expression syntax, parse out flags etc
            char sep = or_match[0];

            string::size_type pos = or_match.find_last_of(sep);
            if ((pos != 0) && (pos != string::npos)) {
                // Main expression
                expression_str = or_match.substr(1, pos - 1);

                // Expression flags
                for (string::size_type i = pos + 1; i < or_match.size(); ++i) {
                    switch (or_match[i]) {
                    case 'i':
                        flags |= REG_ICASE;
                        break;
                    default:
                        cerr << _("Invalid flag for regular expression.\n");
                        break;
                    }
                }
            } else {
                cerr << _("Invalid format of regular expression.\n");
            }

            expression = Util::to_mb_str(expression_str);

        } else {
            expression = Util::to_mb_str(or_match);
        }

        m_reg_ok = !regcomp(&m_o_regex, expression.c_str(), flags);
    } else {
        m_reg_ok = false;
    }

    return m_reg_ok;
}

//! @brief Parses replace part of ed_s command.
//! Expects input in the style of /replace/me/. / can be any character
//! except \. References to sub expressions are made with \num. \0 Represents
//! the part of the string that matched.
bool
RegexString::parse_replace(const std::wstring &or_replace)
{
    m_i_ref_max = 0;

    wstring o_part;
    wstring::size_type o_begin = 0, o_end = 0, o_last = 0;

    // Go through the string and split at \num points
    while ((o_end = or_replace.find_first_of('\\', o_begin)) != string::npos) {
        // Store string between references.
        if (o_end > o_last) {
            o_part = or_replace.substr(o_last, o_end - o_last);
            m_o_ref_list.push_back(RegexString::Part(o_part));
        }

        // Get reference number.
        for (o_begin = ++o_end; isdigit(or_replace[o_end]); o_end++)
            ;

        if (o_end > o_begin) {
            // Convert number and add item.
            o_part = or_replace.substr(o_begin, o_end - o_last);
            int i_ref = strtol(Util::to_mb_str(o_part).c_str(), NULL, 10);
            if (i_ref >= 0) {
                m_o_ref_list.push_back(RegexString::Part(L"", i_ref));
                if (i_ref > m_i_ref_max) {
                    m_i_ref_max = i_ref;
                }
            }
        }

        o_last = o_end;
        o_begin = o_last + 1;
    }

    if (o_begin < or_replace.size()) {
        o_part = or_replace.substr(o_begin, or_replace.size() - o_begin);
        m_o_ref_list.push_back(RegexString::Part(o_part));
    }

    m_i_ref_max++;

    return true;
}

//! @brief Parses ed s style command. /from/to/
bool
RegexString::parse_ed_s(const std::wstring &or_ed_s)
{
    if (or_ed_s.size() < 3) {
        return false;
    }

    wchar_t c_delimeter = or_ed_s[0];
    string::size_type o_middle, o_end;

    // Middle.
    for (o_middle = 1; o_middle < or_ed_s.size(); o_middle++) {
        if ((or_ed_s[o_middle] == c_delimeter)
            && (or_ed_s[o_middle - 1] != '\\')) {
            break;
        }
    }

    // End.
    for (o_end = o_middle + 1; o_end < or_ed_s.size(); o_end++) {
        if ((or_ed_s[o_end] == c_delimeter) && (or_ed_s[o_end - 1] != '\\')) {
            break;
        }
    }

    wstring o_match, o_replace;
    o_match = or_ed_s.substr(1, o_middle - 1);
    o_replace = or_ed_s.substr(o_middle + 1, o_end - o_middle - 1);

    parse_match(o_match);
    parse_replace(o_replace);

    return true;
}

//! @brief Matches RegexString against or_rhs, needs successfull parse_match.
bool
RegexString::operator==(const std::wstring &or_rhs)
{
    if (!m_reg_ok) {
        return false;
    }

    string rhs(Util::to_mb_str(or_rhs));

    return !regexec(&m_o_regex, rhs.c_str(), 0, 0, 0);
}

//! @brief Free resources used by RegexString.
void
RegexString::free_regex(void)
{
    if (m_reg_ok) {
        regfree(&m_o_regex);
        m_reg_ok = false;
    }
}
