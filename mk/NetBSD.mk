.POSIX:

# compiler options
X11_CFLAGS = -I/usr/X11R7/include
X11_LDFLAGS = -L/usr/X11R7/lib -lX11 -lXpm -lXft
PKG_CFLAGS = -I/usr/pkg/include
PKG_LDFLAGS = -L/usr/pkg/lib -lpng16 -ljpeg
