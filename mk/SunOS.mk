.POSIX:

# environment options
AWK = /usr/xpg4/bin/awk
SED = /usr/xpg4/bin/sed
SH = /bin/xpg4/bin/sh

# toolchain
CXX           = CC
CXXFLAGS_BASE = -std=c++03 -O2 -g
LD            = $(CXX)
LDFLAGS_BASE  = -std=c++03

# compiler options
X11_CFLAGS = -I/usr/openwin/include
X11_LDFLAGS = /usr/openwin/lib/libX11.so /usr/openwin/lib/libXext.so /usr/openwin/lib/libXpm.so
PKG_CFLAGS = -I/usr/include
PKG_LDFLAGS = /usr/lib/libpng12.so /usr/lib/libjpeg.so
