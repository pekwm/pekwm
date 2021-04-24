.POSIX:

CXX = g++
CXXFLAGS_BASE = -O2 -g

# compiler options
X11_CFLAGS = -I/usr/X11R6/include -I/usr/X11R6/include/freetype2
X11_LDFLAGS = -L/usr/X11R6/lib -lX11 -lXext -lXpm -lXft -lXrandr
PKG_CFLAGS =
PKG_LDFLAGS =
