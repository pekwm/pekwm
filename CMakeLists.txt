cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/Modules/")

project(pekwm)
set(pekwm_VERSION_MAJOR 0)
set(pekwm_VERSION_MINOR 3)
set(pekwm_VERSION_PATCH 0)
set(pekwm_VERSION "${pekwm_VERSION_MAJOR}.${pekwm_VERSION_MINOR}.${pekwm_VERSION_PATCH}")
set(PEKWM_THEMES "https://www.pekwm.se/themes" CACHE STRING
    "base url where theme index resides, without ending /")

# Check for C++98
set(CMAKE_CXX_STANDARD 98)

include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(GNUInstallDirs)

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
  std::locale num_locale1(\"\");
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
int main() { std::stoi(\"42\"); }
" PEKWM_HAVE_STOI)

check_cxx_source_compiles("
#include <string>
int main() { std::stof(\"42.0\"); }
" PEKWM_HAVE_STOF)

set(CMAKE_CXX_FLAGS ${orig_CMAKE_CXX_FLAGS})

# Look for platform specific methods
check_function_exists(setenv PEKWM_HAVE_SETENV)
check_function_exists(unsetenv PEKWM_HAVE_UNSETENV)
check_function_exists(daemon PEKWM_HAVE_DAEMON)
check_function_exists(clock_gettime PEKWM_HAVE_CLOCK_GETTIME)
check_symbol_exists(timersub sys/time.h PEKWM_HAVE_TIMERSUB)

# Look for modern X11 functions
set(CMAKE_REQUIRED_INCLUDES ${X11_INCLUDE_DIR})
set(CMAKE_EXTRA_INCLUDE_FILES "X11/Intrinsic.h")
check_type_size(Cardinal CARDINAL)
set(CMAKE_EXTRA_INCLUDE_FILES "")

set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})
check_symbol_exists(Xutf8SetWMProperties X11/Xutil.h PEKWM_HAVE_XUTF8)

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
option(ENABLE_XINERAMA "include support for Xinerama" ON)
option(ENABLE_RANDR "include support for Xrandr" ON)
option(ENABLE_XFT "include support for Xft fonts" ON)
option(ENABLE_IMAGE_JPEG "include support for JPEG images" ON)
option(ENABLE_IMAGE_PNG "include support for PNG images" ON)
option(ENABLE_IMAGE_XPM "include support for XPM images" ON)

option(PEDANTIC "turn on strict compile-time warnings" OFF)
option(TESTS "include tests" OFF)

if (ENABLE_XFT)
  find_package(Freetype)
endif (ENABLE_XFT)

if (ENABLE_IMAGE_JPEG)
  find_package(JPEG)
endif (ENABLE_IMAGE_JPEG)

if (ENABLE_IMAGE_PNG)
  find_package(PNG)
endif (ENABLE_IMAGE_PNG)

# Compilation
if (ENABLE_SHAPE AND X11_Xshape_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} XShape")
  set(PEKWM_HAVE_SHAPE 1)
endif (ENABLE_SHAPE AND X11_Xshape_FOUND)

if (ENABLE_XINERAMA AND X11_Xinerama_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} Xinerama")
  set(PEKWM_HAVE_XINERAMA 1)
endif (ENABLE_XINERAMA AND X11_Xinerama_FOUND)

if (ENABLE_XFT AND X11_Xft_FOUND AND FREETYPE_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} Xft")
  set(PEKWM_HAVE_XFT 1)
endif (ENABLE_XFT AND X11_Xft_FOUND AND FREETYPE_FOUND)

if (ENABLE_IMAGE_JPEG AND JPEG_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} image-jpeg")
  set(PEKWM_HAVE_IMAGE_JPEG 1)
endif (ENABLE_IMAGE_JPEG AND JPEG_FOUND)

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} image-png")
  set(PEKWM_HAVE_IMAGE_PNG 1)
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

if (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} image-xpm")
  set(PEKWM_HAVE_IMAGE_XPM 1)
endif (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)

if (ENABLE_RANDR AND X11_Xrandr_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} Xrandr")
  set(PEKWM_HAVE_XRANDR 1)
endif (ENABLE_RANDR AND X11_Xrandr_FOUND)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(pekwm_FEATURES "${pekwm_FEATURES} debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (PEDANTIC)
  set(pekwm_FEATURES "${pekwm_FEATURES} pedantic")
  # -Werror cause the compiler fail with:
  #  Error: original is defined but not used.
  # when throwing some exceptions
  if (CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
  else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")
  endif ()
  check_cxx_compiler_flag("-Wno-variadic-macros" NO_VARIADIC_MACROS)
  if (NO_VARIADIC_MACROS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-variadic-macros")
  endif (NO_VARIADIC_MACROS)
endif (PEDANTIC)

# check after pedantic check, -Wall -Werror -pedantic will cause this
# to fail if missing/ignored
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

if (TESTS)
   enable_testing()
   add_subdirectory(test)
endif (TESTS)
