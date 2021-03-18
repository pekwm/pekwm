//
// Charset.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include <string>

namespace Charset
{
    class WithCharset
    {
    public:
        WithCharset(void);
        ~WithCharset(void);
    };

    void init(void);
    void destruct(void);

    std::string to_mb_str(const std::wstring &str);
    std::wstring to_wide_str(const std::string &str);

    std::string to_utf8_str(const std::wstring &str);
    std::wstring from_utf8_str(const std::string &str);
}
