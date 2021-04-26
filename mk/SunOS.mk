.POSIX:

# environment options
AWK = /usr/xpg4/bin/awk
SED = /usr/xpg4/bin/sed
SH = /usr/xpg4/bin/sh

# toolchain
CXX           = CC
CXXFLAGS_BASE = -std=c++03 -O2 -g $(SUNOS_ARCH_FLAG)
LD            = $(CXX)
LDFLAGS_BASE  = -std=c++03 $(SUNOS_ARCH_FLAG)

# compiler options
ARCH_LIB = $(SUNOS_ARCH_LIB)

X11_CFLAGS = -I/usr/openwin/include
X11_LDFLAGS = /usr/openwin/lib$(ARCH_LIB)/libX11.so /usr/openwin/lib$(ARCH_LIB)/libXext.so /usr/openwin/lib$(ARCH_LIB)/libXpm.so
PKG_CFLAGS = -I/usr/include
PKG_LDFLAGS = /usr/lib$(ARCH_LIB)/libpng12.so /usr/lib$(ARCH_LIB)/libjpeg.so
