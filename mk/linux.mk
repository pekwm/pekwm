.POSIX:

# compiler options
X11_CFLAGS = -I/usr/include
X11_LDFLAGS = -L/usr/lib -lX11 -lXext -lXpm -lXrandr
PKG_CFLAGS = -I/usr/include -I/usr/include/freetype2
PKG_LDFLAGS = -L/usr/lib -lXft -lpng16 -ljpeg
