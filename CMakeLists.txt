cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/Modules/")

project(pekwm)
set(pekwm_VERSION_MAJOR 0)
set(pekwm_VERSION_MINOR 4)
set(pekwm_VERSION_PATCH 0)
set(pekwm_VERSION
    "${pekwm_VERSION_MAJOR}.${pekwm_VERSION_MINOR}.${pekwm_VERSION_PATCH}")
set(PEKWM_THEMES "https://www.pekwm.se/themes" CACHE STRING
    "base url where theme index resides, without ending /")

# Check for C++98
set(CMAKE_CXX_STANDARD 98)

# Export compile commands, LSP integrations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckTypeSize)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckCXXSymbolExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(GNUInstallDirs)
include(FindPkgConfig)
include(FindCURL)

# Look for dependencies
find_package(X11 REQUIRED)

check_include_file_cxx(limits PEKWM_HAVE_LIMITS)
if (NOT PEKWM_HAVE_LIMITS)
  check_include_file(sys/limits.h PEKWM_HAVE_SYS_LIMITS_H)
endif (NOT PEKWM_HAVE_LIMITS)
check_include_file(stdint.h PEKWM_HAVE_STDINT_H)

check_include_file_cxx(locale PEKWM_HAVE_LOCALE)
if (PEKWM_HAVE_LOCALE)
	check_cxx_source_compiles("
#include <locale>
int main() {
  std::locale num_locale(\"\");
  std::locale locale = std::locale().combine<std::numpunct<char> >(num_locale);
}
" PEKWM_HAVE_LOCALE_COMBINE)

else()
	check_include_file(locale.h PEKWM_HAVE_LOCALE_H REQUIRED)
endif (PEKWM_HAVE_LOCALE)

check_function_exists(localtime_r PEKWM_HAVE_LOCALTIME_R REQUIRED)

# Add work-around for missing -std=c++98 flag when running the
# check_cxx_source_compiles tests
set(orig_CMAKE_CXX_FLAGS ${CMAKE_CXX98_FLAGS})
set(CMAKE_CXX_FLAGS ${CMAKE_CXX98_STANDARD_COMPILE_OPTION})

check_cxx_source_compiles("
#include <iostream>
#include <iomanip>
int main() { std::tm tm = {0}; std::cout << std::put_time(&tm, \"%S\"); }
" PEKWM_HAVE_PUT_TIME)

check_cxx_source_compiles("
#include <iostream>
#include <string>
int main() { std::to_string(42); }
" PEKWM_HAVE_TO_STRING)

check_cxx_source_compiles("
#include <string>
int main() { std::stod(\"42.2\"); }
" PEKWM_HAVE_STOD)

check_cxx_source_compiles("
#include <string>
int main() { std::stoi(\"42\"); }
" PEKWM_HAVE_STOI)

check_cxx_source_compiles("
#include <string>
int main() { std::stof(\"42.0\"); }
" PEKWM_HAVE_STOF)

set(CMAKE_CXX_FLAGS ${orig_CMAKE_CXX_FLAGS})

# Look for platform specific methods
check_function_exists(execvpe PEKWM_HAVE_EXECVPE)
check_function_exists(setenv PEKWM_HAVE_SETENV)
check_function_exists(unsetenv PEKWM_HAVE_UNSETENV)
check_function_exists(daemon PEKWM_HAVE_DAEMON)
check_function_exists(clock_gettime PEKWM_HAVE_CLOCK_GETTIME)
check_function_exists(pledge PEKWM_HAVE_PLEDGE)
check_function_exists(timegm PEKWM_HAVE_TIMEGM)
check_cxx_symbol_exists(timersub sys/time.h PEKWM_HAVE_TIMERSUB)
check_cxx_symbol_exists(environ unistd.h PEKWM_HAVE_ENVIRON)

# Look for modern X11 functions
set(CMAKE_REQUIRED_INCLUDES ${X11_INCLUDE_DIR})
set(CMAKE_EXTRA_INCLUDE_FILES "X11/Intrinsic.h")
check_type_size(Cardinal CARDINAL)
set(CMAKE_EXTRA_INCLUDE_FILES "")

set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})
check_cxx_symbol_exists(Xutf8SetWMProperties X11/Xutil.h PEKWM_HAVE_XUTF8)

# Look for platform specific tools
message(STATUS "Looking for program awk")
find_program(AWK awk
             PATHS /usr/pkg/bin /usr/xpg4/bin /usr/local/bin /usr/bin /bin
             NO_DEFAULT_PATH
             REQUIRED)
message(STATUS "Looking for program awk - ${AWK}")

message(STATUS "Looking for program grep")
find_program(GREP ggrep grep
             PATHS /usr/pkg/bin /usr/sfw/bin /usr/local/bin /usr/bin /bin
             NO_DEFAULT_PATH
             REQUIRED)
message(STATUS "Looking for program grep - ${GREP}")

message(STATUS "Looking for program sed")
find_program(SED gsed sed
             PATHS /usr/pkg/bin /usr/sfw/bin /usr/local/bin /usr/bin /bin
             NO_DEFAULT_PATH
             REQUIRED)
message(STATUS "Looking for program sed - ${SED}")

message(STATUS "Looking for program sh")
find_program(SH sh
             PATHS /usr/pkg/bin /usr/xpg4/bin /usr/local/bin /usr/bin /bin
             NO_DEFAULT_PATH
             REQUIRED)
message(STATUS "Looking for program sh - ${SH}")

# Optons
option(ENABLE_SHAPE "include support for Xshape" ON)
option(ENABLE_XDBE "include support for XDBE" ON)
option(ENABLE_XINERAMA "include support for Xinerama" ON)
option(ENABLE_RANDR "include support for Xrandr" ON)
option(ENABLE_XFT "include support for Xft font rendering" ON)
option(ENABLE_PANGO "include support for Pango font rendering" ON)
option(ENABLE_IMAGE_JPEG "include support for JPEG images" ON)
option(ENABLE_IMAGE_PNG "include support for PNG images" ON)
option(ENABLE_IMAGE_XPM "include support for XPM images" ON)
option(ENABLE_CURL "include support for CURL HTTP client" ON)

option(PEDANTIC "turn on strict compile-time warnings" OFF)
option(TESTS "include tests" OFF)
option(DEV "include development tools" OFF)

if (ENABLE_XFT)
  find_package(Freetype)
endif (ENABLE_XFT)

if (ENABLE_PANGO)
	pkg_check_modules(PANGO pango>=1.44)
	pkg_check_modules(PANGOCAIRO pangocairo)
	pkg_check_modules(PANGOXFT pangoxft)
endif (ENABLE_PANGO)

if (ENABLE_IMAGE_JPEG)
	find_package(JPEG)
endif (ENABLE_IMAGE_JPEG)

if (ENABLE_IMAGE_PNG)
	find_package(PNG)
endif (ENABLE_IMAGE_PNG)

# setup compile/link flags, one location for sharing between the different
# compile directories
set(common_INCLUDE_DIRS
    ${PROJECT_BINARY_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/lib
    ${X11_INCLUDE_DIR})
set(common_LIBRARIES ${X11_LIBRARIES})

if (ENABLE_SHAPE AND X11_Xshape_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} XShape")
	set(PEKWM_HAVE_SHAPE 1)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS}
	    ${X11_Xshape_INCLUDE_PATH})
	set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xshape_LIB})
endif (ENABLE_SHAPE AND X11_Xshape_FOUND)

if (ENABLE_XDBE AND X11_Xext_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} XDBE")
	set(PEKWM_HAVE_XDBE 1)
	set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xext_LIB})
endif (ENABLE_XDBE AND X11_Xext_FOUND)

if (ENABLE_XINERAMA AND X11_Xinerama_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} Xinerama")
	set(PEKWM_HAVE_XINERAMA 1)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS}
	    ${X11_Xinerama_INCLUDE_PATH})
	set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xinerama_LIB})
endif (ENABLE_XINERAMA AND X11_Xinerama_FOUND)

if (ENABLE_XFT AND X11_Xft_FOUND AND FREETYPE_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} Xft")
	set(PEKWM_HAVE_XFT 1)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xft_INCLUDE_PATH}
	    ${FREETYPE_INCLUDE_DIRS})
	set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xft_LIB}
	    ${FREETYPE_LIBRARIES})
endif (ENABLE_XFT AND X11_Xft_FOUND AND FREETYPE_FOUND)

if (ENABLE_PANGO AND PANGO_FOUND)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${PANGO_INCLUDE_DIRS})
	set(common_LIBRARIES ${common_LIBRARIES} ${PANGO_LINK_LIBRARIES})
	set(PEKWM_HAVE_PANGO 1)
	if (PANGOCAIRO_FOUND)
		set(pekwm_FEATURES "${pekwm_FEATURES} PangoCairo")
		set(PEKWM_HAVE_PANGO_CAIRO 1)
		set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS}
			${PANGOCAIRO_INCLUDE_DIRS})
		set(common_LIBRARIES ${common_LIBRARIES}
			${PANGOCAIRO_LINK_LIBRARIES})
	endif (PANGOCAIRO_FOUND)
	if (PANGOXFT_FOUND)
		set(pekwm_FEATURES "${pekwm_FEATURES} PangoXft")
		set(PEKWM_HAVE_PANGO_XFT 1)
		set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS}
			${PANGOXFT_INCLUDE_DIRS})
		set(common_LIBRARIES ${common_LIBRARIES}
			${PANGOXFT_LINK_LIBRARIES})
	endif (PANGOXFT_FOUND)
endif (ENABLE_PANGO AND PANGO_FOUND)

if (ENABLE_IMAGE_JPEG AND JPEG_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} image-jpeg")
	set(PEKWM_HAVE_IMAGE_JPEG 1)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})
	set(common_LIBRARIES ${common_LIBRARIES} ${JPEG_LIBRARY})
endif (ENABLE_IMAGE_JPEG AND JPEG_FOUND)

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} image-png")
	set(PEKWM_HAVE_IMAGE_PNG 1)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${PNG_INCLUDE_DIR})
	set(common_LIBRARIES ${common_LIBRARIES} ${PNG_LIBRARY_RELEASE})
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

if (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} image-xpm")
	set(PEKWM_HAVE_IMAGE_XPM 1)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${X11_Xpm_INCLUDE_PATH})
	set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xpm_LIB})
endif (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)

if (ENABLE_RANDR AND X11_Xrandr_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} Xrandr")
	set(PEKWM_HAVE_XRANDR 1)
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS}
	    ${X11_Xrandr_INCLUDE_PATH})
	set(common_LIBRARIES ${common_LIBRARIES} ${X11_Xrandr_LIB})
	set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES} ${X11_Xrandr_LIB})
	check_symbol_exists(XRRGetOutputPrimary X11/extensions/Xrandr.h
			    PEKWM_HAVE_XRRGETOUTPUTPRIMARY)
endif (ENABLE_RANDR AND X11_Xrandr_FOUND)

if (ENABLE_CURL AND CURL_FOUND)
	set(pekwm_FEATURES "${pekwm_FEATURES} curl")
	set(common_INCLUDE_DIRS ${common_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
	set(common_LIBRARIES ${common_LIBRARIES} ${CURL_LIBRARIES})
	set(PEKWM_HAVE_CURL 1)
endif (ENABLE_CURL AND CURL_FOUND)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	set(pekwm_FEATURES "${pekwm_FEATURES} debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall")
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (PEDANTIC)
	set(pekwm_FEATURES "${pekwm_FEATURES} pedantic")
	# Using C++98 with modern headers cause -pedantic to error out,
	# no longer use -Werror for that reason. (long long is C++11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
	check_cxx_compiler_flag("-Wno-variadic-macros" NO_VARIADIC_MACROS)
	if (NO_VARIADIC_MACROS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-variadic-macros")
	endif (NO_VARIADIC_MACROS)
endif (PEDANTIC)

# check after pedantic check, -Wall -pedantic will cause this to fail if
# missing/ignored
check_cxx_source_compiles("
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \"-Wdeprecated-declarations\"
int main() { return 0; }
" PEKWM_HAVE_GCC_DIAGNOSTICS_PUSH)

add_definitions(-DVERSION="${pekwm_VERSION}"
		-DFEATURES="${pekwm_FEATURES}"
		-DBINDIR="${CMAKE_INSTALL_PREFIX}/bin"
		-DDATADIR="${CMAKE_INSTALL_PREFIX}/share"
		-DSYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}/pekwm")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH True)

# Subdirectories
add_subdirectory(contrib)
add_subdirectory(data)
add_subdirectory(doc)
add_subdirectory(src)

if (DEV)
	add_subdirectory(dev)
endif (DEV)

if (TESTS)
	enable_testing()
	add_subdirectory(test)
endif (TESTS)
