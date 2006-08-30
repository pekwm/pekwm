//
// RegexString.cc for pekwm
// Copyright (C)  2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "RegexString.hh"

#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::list;

//! @brief RegexString constructor.
RegexString::RegexString (void) :
        m_reg_ok (false), m_i_ref_max (1)
{
}

//! @brief RegexString destructor.
RegexString::~RegexString (void)
{
    free_regex ();
}

//! @brief Simple ed s command lookalike.
bool
RegexString::ed_s (std::string &or_string)
{
    if (!m_reg_ok)
        return false;

    const char *op_str = or_string.c_str();
    regmatch_t *op_matches = new regmatch_t[m_i_ref_max];

    if (regexec (&m_o_regex, or_string.c_str (), m_i_ref_max, op_matches, 0))
    {
        delete [] op_matches;
        return false;
    }

    string o_result;
    uint i_ref, i_size;

    list<RegexString::Part>::iterator it(m_o_ref_list.begin ());
    for (; it != m_o_ref_list.end(); ++it)
    {
        if (it->get_reference () >= 0)
        {
            i_ref = it->get_reference ();

            if (op_matches[i_ref].rm_so != -1)
            {
                i_size = op_matches[i_ref].rm_eo - op_matches[i_ref].rm_so;
                o_result.append (string (op_str + op_matches[i_ref].rm_so, i_size));
            }
        }
        else
            o_result.append (it->get_string ());
    }

    // Replace area regexp matched.
    i_size = op_matches[0].rm_eo - op_matches[0].rm_so;
    or_string.replace (op_matches[0].rm_so, i_size, o_result);

    return true;
}

//! @brief Parses match part of regular expression.
bool
RegexString::parse_match (const std::string &or_match)
{
    if (or_match.size ())
    {
        if (m_reg_ok)
            free_regex ();
        m_reg_ok = !regcomp (&m_o_regex, or_match.c_str (), REG_EXTENDED);
    }
    else
        m_reg_ok = false;

    return m_reg_ok;
}

//! @brief Parses replace part of ed_s command.
//! Expects input in the style of /replace/me/. / can be any character
//! except \. References to sub expressions are made with \num. \0 Represents
//! the part of the string that matched.
bool
RegexString::parse_replace (const std::string &or_replace)
{
    m_i_ref_max = 0;

    string o_part;
    string::size_type o_begin = 0, o_end = 0, o_last = 0;

    // Go through the string and split at \num points
    while ((o_end = or_replace.find_first_of ('\\', o_begin)) != string::npos)
    {
        // Store string between references.
        if (o_end > o_last)
        {
            o_part = or_replace.substr (o_last, o_end - o_last);
            m_o_ref_list.push_back (RegexString::Part (o_part));
        }

        // Get reference number.
        for (o_begin = ++o_end; isdigit (or_replace[o_end]); o_end++)
            ;

        if (o_end > o_begin)
        {
            // Convert number and add item.
            o_part = or_replace.substr (o_begin, o_end - o_last);
            int i_ref = strtol (o_part.c_str (), NULL, 10);
            if (i_ref >= 0)
            {
                m_o_ref_list.push_back (RegexString::Part ("", i_ref));
                if (i_ref > m_i_ref_max)
                    m_i_ref_max = i_ref;
            }
        }

        o_last = o_end;
        o_begin = o_last + 1;
    }

    if (o_begin < or_replace.size ())
    {
        o_part = or_replace.substr (o_begin, or_replace.size () - o_begin);
        m_o_ref_list.push_back (RegexString::Part (o_part));
    }

    m_i_ref_max++;

    return true;
}

//! @brief Parses ed s style command. /from/to/
bool
RegexString::parse_ed_s (const std::string &or_ed_s)
{
    if (or_ed_s.size () < 3)
        return false;

    char c_delimeter = or_ed_s[0];
    string::size_type o_middle, o_end;

    // Middle.
    for (o_middle = 1; o_middle < or_ed_s.size (); o_middle++)
    {
        if ((or_ed_s[o_middle] == c_delimeter) && (or_ed_s[o_middle - 1] != '\\'))
            break;
    }

    // End.
    for (o_end = o_middle + 1; o_end < or_ed_s.size (); o_end++)
    {
        if ((or_ed_s[o_end] == c_delimeter) && (or_ed_s[o_end - 1] != '\\'))
            break;
    }

    string o_match, o_replace;
    o_match = or_ed_s.substr (1, o_middle - 1);
    o_replace = or_ed_s.substr (o_middle + 1, o_end - o_middle - 1);

    parse_match (o_match);
    parse_replace (o_replace);

    return true;
}

//! @brief Matches RegexString against or_rhs, needs successfull parse_match.
bool
RegexString::operator== (const std::string &or_rhs)
{
    if (!m_reg_ok)
        return false;

    return !regexec (&m_o_regex, or_rhs.c_str (), 0, 0, 0);
}

//! @brief Free resources used by RegexString.
void
RegexString::free_regex (void)
{
    if (m_reg_ok)
    {
        regfree (&m_o_regex);
        m_reg_ok = false;
    }
}
