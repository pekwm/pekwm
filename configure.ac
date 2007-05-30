m4_pattern_allow(AM_CONFIG_HEADERS)

dnl init autotools
AC_INIT(pekwm, SVN)
AC_CONFIG_SRCDIR(src/main.cc)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)

dnl Check for programs
AC_PROG_CXX
AC_PROG_INSTALL

AC_LANG_CPLUSPLUS

AC_CHECK_PROGS(regex_cmd, sed)
if test x$regex_cmd = "x"; then
	AC_MSG_ERROR([Could not find sed. It is required to build the configuration files.])
fi

dnl add x11 to the env
AC_PATH_X
AC_PATH_XTRA

CXXFLAGS="$CXXFLAGS $X_CFLAGS"
LIBS="$LIBS $X_LIBS"
LDFLAGS="$LDFLAGS $LIBS $X_PRE_LIBS"

dnl Check for libraries
AC_CHECK_LIB(X11, XOpenDisplay,
	LIBS="$LIBS -lX11",
	AC_MSG_ERROR([Could not find XOpenDisplay in -lX11.])
)

dnl Check for XShape extension support and proper library files
enableval="yes"
AC_MSG_CHECKING([whether to build support for the XShape extension])
AC_ARG_ENABLE(shape,
	AC_HELP_STRING([--enable-shape],
								 [enable support of the XShape extension [default=yes]]), ,
							[shape_enable=yes])
if test x$enableval = "xyes"; then
  AC_MSG_RESULT([yes])
  AC_CHECK_LIB(Xext, XShapeCombineShape,
    AC_MSG_CHECKING([for X11/extensions/shape.h])
    AC_TRY_COMPILE(
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
      , long foo = ShapeSet,
			AC_MSG_RESULT([yes])
			AC_DEFINE(HAVE_SHAPE, [1], [Define to 1 if you have an XShape capable server])
			LIBS="$LIBS -lXext"
			FEATURES="$FEATURES XShape",
		AC_MSG_RESULT([no])))
else
  AC_MSG_RESULT([no])
  CONFIGOPTS="$CONFIGOPTS --disable-shape"
fi

dnl Check for Xinerama support
AC_MSG_CHECKING([whether to build support for the Xinerama extension])
AC_ARG_ENABLE(xinerama,
	AC_HELP_STRING([--enable-xinerama],	
								 [enable xinerama extension [default=no]]), ,
							[enable_xinerama=no])
if test "x$enable_xinerama" = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB(Xinerama, XineramaQueryScreens,
			AC_DEFINE(HAVE_XINERAMA, [1], [Define to 1 if you want Xinerama support to be])
			LIBS="$LIBS -lXinerama"
			FEATURES="$FEATURES Xinerama")
else
	AC_MSG_RESULT([no])
fi

dnl Check for XRANDR support
AC_MSG_CHECKING([wheter to build support for the XRANDR extension])
AC_ARG_ENABLE(xrandr,
	AC_HELP_STRING([--enable-xrandr],
								 [enable support for the XRANDR extension [default=yes]]), ,
							[enable_xrandr=yes])
if test "x$enable_xrandr" = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB(Xrandr, XRRQueryExtension,
		AC_DEFINE(HAVE_XRANDR, [1], [Define to 1 if you have an XRANDR capable server])
		LIBS="$LIBS -lXrandr"
		FEATURES="$FEATURES Xrandr")
else
 	AC_MSG_RESULT([no])
fi

dnl Check for Xft support
AC_MSG_CHECKING([whether to support Xft fonts])
AC_ARG_ENABLE(xft,
	AC_HELP_STRING([--enable-xft],
								 [enable Xft font support [default=yes]]), ,
							[enable_xft=yes])
if test "x$enable_xft" = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_PATH_GENERIC(xft, 2.0.0,
		[
			AC_DEFINE(HAVE_XFT, [1], [Define to 1 if you want Xft2 font support])
			XFT_LIBS=`xft-config --libs`
			XFT_CFLAGS=`xft-config --cflags`
			LIBS="$LIBS $XFT_LIBS"
			CXXFLAGS="$CXXFLAGS $XFT_CFLAGS"
			FEATURES="$FEATURES Xft";
		],
		[
			AC_MSG_WARN([Couldn't fint Xft.])
		])
else
	AC_MSG_RESULT([no])
fi

dnl Check for pcre support
AC_MSG_CHECKING([whether to use pcre for autoprops])
AC_ARG_ENABLE(pcre,
	AC_HELP_STRING([--enable-pcre],
								 [enable pcre regex [default=no]]), ,
							[enable_pcre=no])
if test "x$enable_pcre" = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB(pcre, pcre_compile,
			AC_DEFINE(HAVE_PCRE, [1], [Define to 1 if you want PCRE style regexps])
			PCRE_LIBS=`pcre-config --libs-posix`
			PCRE_CFLAGS=`pcre-config --cflags-posix`
			LIBS="$LIBS $PCRE_LIBS"
			CXXFLAGS="$CXXFLAGS $PCRE_CFLAGS"
			FEATURES="$FEATURES pcre")
else
	AC_MSG_RESULT([no])
fi

dnl Check for XPM support
AC_MSG_CHECKING([wheter to build support XPM images])
AC_ARG_ENABLE(image-xpm,
	AC_HELP_STRING([--enable-image-xpm],
								 [enable support for XPM images [default=yes]]), ,
							[enableval=yes])
if test "x$enableval" = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB(Xpm, XpmReadFileToPixmap,
		AC_MSG_CHECKING([for X11/xpm.h])
		AC_TRY_COMPILE(
#include <X11/xpm.h>
			, int foo = XpmSuccess,
			AC_MSG_RESULT([yes])
			AC_DEFINE(HAVE_IMAGE_XPM, [1], [Define to 1 if you libXpm])
			LIBS="$LIBS -lXpm"
			FEATURES="$FEATURES image-xpm",
		AC_MSG_RESULT([no])))
else
	AC_MSG_RESULT([no])
fi

dnl Check for JPEG support
AC_MSG_CHECKING([wheter to build support for JPEG images])
AC_ARG_ENABLE(image-jpeg,
	AC_HELP_STRING([--enable-image-jpeg],
								 [enable support for JPEG images [default=yes]]), ,
							[enableval=yes])
if test "x$enableval" = "xyes"; then
 	AC_MSG_RESULT([yes])
 	AC_CHECK_LIB(jpeg, jpeg_read_header,
 		AC_MSG_CHECKING([for jpeglib.h])
 		AC_TRY_CPP([#include <jpeglib.h>],
			jpeg_ok=yes,
 			jpeg_ok=no)
 		AC_MSG_RESULT($jpeg_ok)
 		if test "$jpeg_ok" = yes; then
			AC_DEFINE(HAVE_IMAGE_JPEG, [1], [Define to 1 if you have jpeg6b])
 			LIBS="$LIBS -ljpeg"
 			FEATURES="$FEATURES image-jpeg"
 		fi,
 		AC_MSG_RESULT([no]))
else
	AC_MSG_RESULT([no])
fi

dnl Check for PNG support
AC_MSG_CHECKING([wheter to build support for PNG images])
AC_ARG_ENABLE(image-png,
	AC_HELP_STRING([--enable-image-png],
								 [enable support for PNG images [default=yes]]), ,
							[enableval=yes])
if test "x$enableval" = "xyes"; then
 	AC_MSG_RESULT([yes])
 	AC_CHECK_LIB(png, png_read_info,
 		AC_MSG_CHECKING([for png.h])
 		AC_TRY_CPP([#include <png.h>],
			png_ok=yes,
 			png_ok=no)
 		AC_MSG_RESULT($png_ok)
 		if test "$png_ok" = yes; then
			AC_DEFINE(HAVE_IMAGE_PNG, [1], [Define to 1 if you have libpng])
 			LIBS="$LIBS -lpng"
 			FEATURES="$FEATURES image-png"
 		fi,
 		AC_MSG_RESULT([no]))
else
	AC_MSG_RESULT([no])
fi

dnl Check for header files
AC_STDC_HEADERS
AC_CHECK_HEADERS([limits])

dnl Check for library functions
AC_CHECK_FUNC(setenv, [AC_DEFINE(HAVE_SETENV, [1], [Define to 1 if you the setenv systam call])], )

dnl Check simple toggles ( no outer dependecies )

dnl Check whether to include debugging code
AC_MSG_CHECKING([whether to include verbose debugging code])
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug],
								 [include verbose debugging code [default=no]]), ,
							[enable_debug=no])
if test "x$enable_debug" = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(DEBUG, [1], [Define to 1 to compile in debug information])
	FEATURES="$FEATURES debug"
else
	AC_MSG_RESULT([no])
fi

dnl Check wheter to use strict warnings
AC_MSG_CHECKING([whether to use strict compile-time warnings])
AC_ARG_ENABLE(pedantic,
	AC_HELP_STRING([--enable-pedantic],
								 [turn on strict compile-time warnings [default=no]]), ,
							[enable_pedantic=no])
if test "$enable_pedantic" = "yes"; then
	AC_MSG_RESULT([yes])
	if test "$CXX" = "g++"; then
		CXXFLAGS="-Wall -Werror -pedantic $CXXFLAGS"
	fi
	FEATURES="$FEATURES pedantic"
else
	AC_MSG_RESULT([no])
fi

dnl Check whether to use menus
AC_MSG_CHECKING([whether to include menu support])
AC_ARG_ENABLE(menus,
  AC_HELP_STRING([--enable-menus],
								 [include menu support [default=yes]]), ,
							[menus_enable=yes])
if test x$enableval = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(MENUS, [1], [Define to 1 to compile in menu support])
	FEATURES="$FEATURES menus"
else
	AC_MSG_RESULT([no])
fi

dnl Check whether to include harbour
AC_MSG_CHECKING([whether to include harbour])
AC_ARG_ENABLE(harbour,
	AC_HELP_STRING([--enable-harbour],
								 [include harbour [default=yes]]), ,
							[harbour_enable=yes])
if test x$enableval = "xyes"; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(HARBOUR, [1], [Define to 1 to compile in support for dockapps])
	FEATURES="$FEATURES harbour"
else
	AC_MSG_RESULT([no])
fi

dnl define build info
EVO=`date`
AC_DEFINE_UNQUOTED(FEATURES, "$FEATURES", [Build info, do not touch])
AC_DEFINE_UNQUOTED(EXTRA_VERSION_INFO, " Built on $EVO", [Build info, do not touch])

dnl create the makefiles
AC_OUTPUT(Makefile
				src/Makefile
				data/Makefile
				data/themes/Makefile
				data/themes/default/Makefile
				data/scripts/Makefile
				doc/Makefile)

dnl print results
AC_MSG_RESULT([*])
AC_MSG_RESULT([* $PACKAGE version $VERSION configured successfully.])
AC_MSG_RESULT([*])
AC_MSG_RESULT([* PREFIX: $prefix])
AC_MSG_RESULT([* FEATURES: $FEATURES])
AC_MSG_RESULT([* CXXFLAGS: $CXXFLAGS])
AC_MSG_RESULT([* LIBS: $LIBS])
AC_MSG_RESULT([*])
