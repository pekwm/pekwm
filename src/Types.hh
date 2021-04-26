//
// Types.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_TYPES_HH_
#define _PEKWM_TYPES_HH_

extern "C" {
#include <sys/types.h>
#ifdef PEKWM_HAVE_STDINT_H
#include <stdint.h>
#endif // PEKWM_HAVE_STDINT_H
}

#ifndef ushort
#define ushort unsigned short
#endif // ushort

#ifndef uint
#define uint unsigned int
#endif // uint

#ifndef ulong
#define ulong unsigned long
#endif // ulong

#ifndef uchar
#define uchar unsigned char
#endif // uchar

#endif // _PEKWM_TYPES_HH_
