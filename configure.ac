AC_INIT([pekwm], [0.4.0], [pekdon@gmail.com])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_CXX
AC_PROG_GREP
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_SED
PKG_PROG_PKG_CONFIG

AC_ARG_ENABLE(tests,
	      [AS_HELP_STRING([--enable-tests], [Enable compilation of tests])],
	      [TESTS=yes], [TESTS=no])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$TESTS" = "xyes"])

dnl Check for /usr/xpg4/bin/sh and use if available to make shipped scripts
dnl work as expected under Solaris
AC_CHECK_PROG([PEKWM_SH], [sh], [/usr/xpg4/bin/sh], [/bin/sh], [/usr/xpg4/bin])

dnl Host type conditionals
AC_MSG_CHECKING([target operating system])
case $target_os in
	freebsd*)
		PEKWM_HOST="FreeBSD"
		;;
	linux*)
		PEKWM_HOST="Linux"
		;;
	netbsd*)
		PEKWM_HOST="NetBSD"
		;;
	openbsd*)
		PEKWM_HOST="OpenBSD"
		;;
	solaris*)
		PEKWM_HOST="SunOS"
		;;
	*)
		PEKWM_HOST="Other"
		;;
esac
AC_MSG_RESULT([$PEKWM_HOST])
AM_CONDITIONAL([PEKWM_HOST_SUNOS], [test "x$PEKWM_HOST" = "xSunOS"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile
		 contrib/Makefile
		 data/Makefile
		 data/icons/Makefile
		 data/scripts/Makefile
		 data/themes/Makefile
		 data/themes/default/Makefile
		 dev/Makefile
		 doc/Makefile
		 src/Makefile
		 src/bg/Makefile
		 src/cfg/Makefile
		 src/ctrl/Makefile
		 src/dialog/Makefile
		 src/lib/Makefile
		 src/panel/Makefile
		 src/screenshot/Makefile
		 src/sys/Makefile
		 src/tk/Makefile
		 src/wm/Makefile
		 test/Makefile
		 test/system/Makefile])

AC_PATH_XTRA

AC_CHECK_HEADER([sys/limits.h],
		[AC_DEFINE([PEKWM_HAVE_SYS_LIMITS_H], [1],
			   [Define to 1 if sys/limits.h is available])])
AC_CHECK_HEADER([locale.h],
		[AC_DEFINE([PEKWM_HAVE_LOCALE_H], [1],
			   [Define to 1 if locale.h is available])])
AC_CHECK_HEADER([limits.h],
		[AC_DEFINE([PEKWM_HAVE_LIMITS_H], [1],
			   [Define to 1 if limits.h is available])])
AC_CHECK_HEADER([stdint.h],
		[AC_DEFINE([PEKWM_HAVE_STDINT_H], [1],
			   [Define to 1 if stdint.h is available])])

AC_CHECK_FUNC(clock_gettime,
	      [AC_DEFINE(PEKWM_HAVE_CLOCK_GETTIME, [1],
			 [Define to 1 if clock_gettime is available])
	       CLOCK_GETTIME_FOUND=yes],
	      [CLOCK_GETTIME_FOUND=no])
if test "x$CLOCK_GETTIME_FOUND" = "xno"; then
	AC_CHECK_LIB([rt], [clock_gettime],
		     [AC_DEFINE([PEKWM_HAVE_CLOCK_GETTIME], [1],
			 [Define to 1 if clock_gettime is available])
		      rt_LIB="-lrt"])
fi
AC_CHECK_FUNC(daemon, [AC_DEFINE([PEKWM_HAVE_DAEMON], [1],
				 [Define to 1 if daemon is available])])
AC_CHECK_FUNC(execvpe, [AC_DEFINE(PEKWM_HAVE_EXECVPE, [1],
				  [Define to 1 if execvpe is available])])
AC_CHECK_FUNC(localtime_r,
	      [AC_DEFINE(PEKWM_HAVE_LOCALTIME_R, [1],
			 [Define to 1 if localtime_r is available])])
AC_CHECK_FUNC(pledge, [AC_DEFINE([PEKWM_HAVE_PLEDGE], [1],
				 [Define to 1 if pledge is available])])

AC_CHECK_FUNC(setenv, [AC_DEFINE([PEKWM_HAVE_SETENV], [1],
				 [Define to 1 if setenv is available])])
AC_CHECK_FUNC(timegm, [AC_DEFINE([PEKWM_HAVE_TIMEGM], [1],
				 [Define to 1 if timegm is available])])
AC_CHECK_FUNC(unsetenv, [AC_DEFINE([PEKWM_HAVE_UNSETENV], [1],
				   [Define to 1 if unsetenv is available])])

AC_CHECK_LIB([X11], [Xutf8SetWMProperties],
	      [AC_DEFINE([PEKWM_HAVE_XUTF8], [1],
			 [Define to 1 if Xutf8SetWMProperties is available])])

AC_MSG_CHECKING([for environ in unistd.h])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([#include <unistd.h>],
			 [const char **tmp = environ;])],
	[AC_DEFINE([PEKWM_HAVE_ENVIRON], 1,
		   [Define to 1 if environ is available])
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for timersub in sys/time.h])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([#include <sys/time.h>],
			 [struct timeval tv; timersub(&tv, &tv, &tv);])],
	[AC_DEFINE([PEKWM_HAVE_TIMERSUB], 1,
		   [Define to 1 if timersub is available])
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])

dnl library dependencies
PKG_CHECK_EXISTS([libcurl >= 1.0.0], [LIBCURL_FOUND=yes], [LIBCURL_FOUND=no])
if test "x$LIBCURL_FOUND" = "xyes"; then
	PKG_CHECK_MODULES([libcurl], [libcurl >= 1.0.0],
			  [AC_DEFINE([PEKWM_HAVE_CURL], [1],
				     [Define to 1 if libcurl is available])
			   FEATURES="$FEATURES curl"]
			  [])
fi

PKG_CHECK_MODULES([libjpeg], [libjpeg >= 1.0.0],
		  [AC_DEFINE([PEKWM_HAVE_IMAGE_JPEG], [1],
			     [Define to 1 if libjpeg is available])
		   LIBJPEG_FOUND=yes
		   FEATURES="$FEATURES image-jpeg"],
		  [LIBJPEG_FOUND=no])
if test "x$LIBJPEG_FOUND" = "xno"; then
        AC_CHECK_LIB([jpeg], [jpeg_read_header],
		     [AC_MSG_CHECKING([for jpeglib.h])
		      AC_PREPROC_IFELSE(
			[AC_LANG_PROGRAM([[#include <jpeglib.h>]], [])],
			[LIBJPEG_FOUND=yes], [])])
        AC_MSG_RESULT($LIBJPEG_FOUND)
        if test "$LIBJPEG_FOUND" = yes; then
		AC_DEFINE(PEKWM_HAVE_IMAGE_JPEG, [1],
			  [Define to 1 if libjpeg is available])
		AC_SUBST([libjpeg_LIBS], ["-ljpeg"])
		FEATURES="$FEATURES image-jpeg"
	fi
fi
AM_CONDITIONAL([PEKWM_WITH_IMAGE_JPEG], [test "x$LIBJPEG_FOUND" = "xyes"])

PKG_CHECK_MODULES([libpng], [libpng >= 1.0.0],
		  [AC_DEFINE([PEKWM_HAVE_IMAGE_PNG], [1],
			     [Define to 1 if libpng is available])
		   LIBPNG_FOUND=yes
		   FEATURES="$FEATURES image-png"],
		  [LIBPNG_FOUND=no])
AM_CONDITIONAL([PEKWM_WITH_IMAGE_PNG], [test "x$LIBPNG_FOUND" = "xyes"])

PKG_CHECK_MODULES([xpm], [xpm >= 1.0.0],
		  [AC_DEFINE([PEKWM_HAVE_IMAGE_XPM], [1],
			     [Define to 1 if xpm is available])
		   XPM_FOUND=yes
		   FEATURES="$FEATURES image-xpm"],
		  [XPM_FOUND=no])
AM_CONDITIONAL([PEKWM_WITH_IMAGE_XPM], [test "x$XPM_FOUND" = "xyes"])

PKG_CHECK_MODULES([pango], [pango >= 1.44.0],
		  [AC_DEFINE([PEKWM_HAVE_PANGO], [1],
			     [Define to 1 if pango is available])
		   PANGO_FOUND=yes],
		  [PANGO_FOUND=no])
AM_CONDITIONAL([PEKWM_WITH_PANGO], [test "x$PANGO_FOUND" = "xyes"])

PKG_CHECK_MODULES([pangocairo], [pangocairo >= 1.44.0],
		  [AC_DEFINE([PEKWM_HAVE_PANGO_CAIRO], [1],
			     [Define to 1 if pangocairo is available])
		   PANGO_CAIRO_FOUND=yes
		   FEATURES="$FEATURES PangoCairo"],
		  [PANGO_CAIRO_FOUND=no])
AM_CONDITIONAL([PEKWM_WITH_PANGO_CAIRO], [test "x$PANGO_CAIRO_FOUND" = "xyes"])

PKG_CHECK_MODULES([pangoxft], [pangoxft >= 1.44.0],
		  [AC_DEFINE([PEKWM_HAVE_PANGO_XFT], [1],
			     [Define to 1 if pangoxft is available])
		   PANGO_XFT_FOUND=yes
		   FEATURES="$FEATURES PangoXft"],
		  [PANGO_XFT_FOUND=no])
AM_CONDITIONAL([PEKWM_WITH_PANGO_XFT], [test "x$PANGO_XFT_FOUND" = "xyes"])

PKG_CHECK_MODULES([xext], [xext >= 1.0.0],
		  [AC_DEFINE([PEKWM_HAVE_XDBE], [1],
			     [Define to 1 if XDBE is available])
		   AC_DEFINE([PEKWM_HAVE_SHAPE], [1],
			     [Define to 1 if XShape is available])
		   FEATURES="$FEATURES XShape XDBE"],
		  [XEXT_FOUND=no])

PKG_CHECK_MODULES([xft], [xft >= 1.0.0],
		  [AC_DEFINE([PEKWM_HAVE_XFT], [1],
			     [Define to 1 if xft is available])
		   XFT_FOUND=yes
		   FEATURES="$FEATURES Xft"],
		  [XFT_FOUND=no])
AM_CONDITIONAL([PEKWM_WITH_XFT], [test "x$XFT_FOUND" = "xyes"])

PKG_CHECK_MODULES([xinerama], [xinerama >= 1.0.0],
		  [AC_DEFINE([PEKWM_HAVE_XRANDR], [1],
			     [Define to 1 if xrandr is available])
		   FEATURES="$FEATURES Xinerama"],
		  [XINERAMA_FOUND=no])

PKG_CHECK_MODULES([xrandr], [xrandr >= 1.0.0],
		  [AC_DEFINE([PEKWM_HAVE_XRANDR], [1],
			     [Define to 1 if xrandr is available])
		   XRANDR_FOUND=yes
		   FEATURES="$FEATURES Xrandr"],
		  [XRANDR_FOUND=no])
if test "x$XRANDR_FOUND" = "xyes"; then
	AC_CHECK_LIB([Xrandr], [XRRGetOutputPrimary],
		     [AC_DEFINE([PEKWM_HAVE_XRRGETOUTPUTPRIMARY], [1],
			 [Define to 1 if XRRGetOutputPrimary is available])])
fi
AM_CONDITIONAL([PEKWM_WITH_XRANDR], [test "x$XRANDR_FOUND" = "xyes"])

PKG_CHECK_MODULES([libudev], [libudev >= 100],
		  [AC_DEFINE([PEKWM_HAVE_LIBUDEV], [1],
			     [Define to 1 if libudev is available])
		   LIBUDEV_FOUND=yes
		   FEATURES="$FEATURES libudev"],
		  [LIBUDEV_FOUND=no])
AM_CONDITIONAL([PEKWM_WITH_LIBUDEV], [test "x$LIBUDEV_FOUND" = "xyes"])

AC_LANG_PUSH([C++])

AC_CHECK_HEADER([limits],
		[AC_DEFINE([PEKWM_HAVE_LIMITS], [1],
			   [Define to 1 if limits is available])])

AC_MSG_CHECKING([for std::locale().combine])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <locale>],
			 [std::locale num_locale("");
			  std::locale locale = std::locale()
				.combine<std::numpunct<char> >(num_locale);])],
	[AC_DEFINE([PEKWM_HAVE_LOCALE_COMBINE], [1],
		   [Define to 1 if std::put_time is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for std::put_time])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <iostream>
			  #include <iomanip>],
			  [std::tm tm = {0};
			   std::cout << std::put_time(&tm, "%S");])],
	[AC_DEFINE([PEKWM_HAVE_PUT_TIME], [1],
		   [Define to 1 if std::put_time is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for std::to_string])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <iostream>
			  #include <string>],
			  [std::cout << std::to_string(42);])],
	[AC_DEFINE([PEKWM_HAVE_TO_STRING], [1],
		   [Define to 1 if std::to_string is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for std::stod])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <string>],
			 [double val = std::stod("42.0");])],
	[AC_DEFINE([PEKWM_HAVE_STOD], [1],
		   [Define to 1 if std::stod is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for std::stoi])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <string>],
			 [int val = std::stoi("42");])],
	[AC_DEFINE([PEKWM_HAVE_STOI], [1],
		   [Define to 1 if std::stoi is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for std::stof])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <string>],
			 [float val = std::stof("42.0");])],
	[AC_DEFINE([PEKWM_HAVE_STOF], [1],
		   [Define to 1 if std::stoi is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_MSG_CHECKING([for NAN])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([extern "C" {
			  #include <math.h>
			  }],
			  [double val = NAN;])],
	[AC_DEFINE([PEKWM_HAVE_NAN], [1],
		   [Define to 1 if NAN is defined])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_LANG_POP([C++])

AC_SUBST(LIB_CFLAGS,
	 "$X_CFLAGS $xext_CFLAGS $xinerama_CFLAGS $xrandr_CFLAGS")
AC_SUBST(LIB_LIBS,
	 "$rt_LIB $X_LIBS -lX11 $xext_LIBS $xinerama_LIBS $xrandr_LIBS")
TK_CFLAGS="$libjpeg_CFLAGS $libpng_CFLAGS $xpm_CFLAGS $xft_CFLAGS"
TK_CFLAGS="$TK_CFLAGS $pango_CFLAGS $pangocairo_CFLAGS $pangoxft_CFLAGS"
AC_SUBST(TK_CFLAGS, "$TK_CFLAGS")
TK_LIBS="$libjpeg_LIBS $libpng_LIBS $xpm_LIBS $xft_LIBS"
TK_LIBS="$TK_LIBS $pango_LIBS $pangocairo_LIBS $pangoxft_LIBS"
AC_SUBST(TK_LIBS, "$TK_LIBS")

CXXFLAGS="$CXXFLAGS -DBINDIR=\\\"$bindir\\\""
CXXFLAGS="$CXXFLAGS -DSYSCONFDIR=\\\"$sysconfdir/pekwm\\\""
CXXFLAGS="$CXXFLAGS -DDATADIR=\\\"$datadir\\\""
CXXFLAGS="$CXXFLAGS -DPEKWM_SH=\\\"$PEKWM_SH\\\""

dnl Features reported in pekwm --info
AC_SUBST(PEKWM_FEATURES, "$FEATURES")
dnl base url where theme index resides, without ending /
AC_SUBST(PEKWM_THEMES, "https://www.pekwm.se/themes")

AC_OUTPUT
m4_unquote(
  _m4_defn([_m4_wrap_text])_m4_popdef([_m4_wrap_text]))
