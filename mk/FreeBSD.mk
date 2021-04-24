.POSIX:

# compiler options
X11_CFLAGS = -I/usr/local/include
X11_LDFLAGS = -L/usr/local/lib -lX11 -lXext -lXpm -lXrandr -lXft
PKG_CFLAGS = -I/usr/local/include -I/usr/local/include/freetype2
PKG_LDFLAGS = -L/usr/local/lib -lpng16 -ljpeg
