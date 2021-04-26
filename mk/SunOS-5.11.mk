.POSIX:

# environment options
AWK = /usr/xpg4/bin/awk
SED = /usr/xpg4/bin/sed
SH = /usr/xpg4/bin/sh

# toolchain
CXX           = CC
CXXFLAGS_BASE = -std=c++03 -O2 -g
LD            = $(CXX)
LDFLAGS_BASE  = -std=c++03

# compiler options
X11_CFLAGS = -I/usr/X11R7/include
X11_LDFLAGS = /usr/lib/libX11.so /usr/lib/libXext.so /usr/lib/libXpm.so /usr/lib/libXrandr.so /usr/lib/libXft.so
PKG_CFLAGS = -I/usr/include -I/usr/include/freetype2
PKG_LDFLAGS = /usr/lib/libpng14.so /usr/lib/libjpeg.so
