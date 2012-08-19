//
// Atoms.cc for pekwm
// Copyright © 2003-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Atoms.hh"
#include "x11.hh"
#include "Util.hh"

extern "C" {
#include <X11/Xutil.h>
}

using std::string;

namespace AtomUtil {

//! @brief Get unknown property
bool
getProperty(Window win, Atom atom, Atom type,
            ulong expected, uchar** data, ulong *actual)
{
    Atom r_type;
    int r_format, status;
    ulong read = 0, left = 0;

    *data = 0;
    do {
        if (*data) {
            XFree(*data);
            *data = 0;
        }
        expected += left;

        status =
            XGetWindowProperty(X11::getDpy(), win, atom,
                               0L, expected, False, type,
                               &r_type, &r_format, &read, &left, data);

        if (status != Success || type != r_type || read == 0) {
            if (*data) {
                XFree(*data);
                *data = 0;
            }
            left = 0;
        }
    } while (left);

    if (actual) {
        *actual = read;
    }

    return (*data != 0);
}

/**
 * Read text property.
 *
 * @param win Window to get property from.
 * @param atom Atom holding the property.
 * @param value Return string.
 * @return true if property was successfully read.
 */
bool
getTextProperty(Window win, Atom atom, std::string &value)
{
    // Read text property, return if it fails.
    XTextProperty text_property;
    if (! XGetTextProperty(X11::getDpy(), win, &text_property, atom)
        || ! text_property.value || ! text_property.nitems) {
        return false;
    }

    if (text_property.encoding == XA_STRING) {
        value = reinterpret_cast<const char*>(text_property.value);
    } else {
        char **mb_list;
        int num;

        XmbTextPropertyToTextList(X11::getDpy(), &text_property, &mb_list, &num);
        if (mb_list && num > 0) {
            value = *mb_list;
            XFreeStringList(mb_list);
        }
    }

    XFree(text_property.value);

    return true;
}

//! @brief
void*
getEwmhPropData(Window win, Atom prop, Atom type, int &num)
{
    Atom type_ret;
    int format_ret;
    ulong items_ret, after_ret;
    uchar *prop_data = 0;

    XGetWindowProperty(X11::getDpy(), win, prop, 0, 0x7fffffff,
                       False, type, &type_ret, &format_ret, &items_ret,
                       &after_ret, &prop_data);

    num = items_ret;

    return prop_data;
}

} // end namespace AtomUtil
