cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/Modules/")
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)

project(pekwm)
set(pekwm_VERSION_MAJOR 0)
set(pekwm_VERSION_MINOR 1)
set(pekwm_VERSION_PATCH 18-rc1)
set(pekwm_VERSION "${pekwm_VERSION_MAJOR}.${pekwm_VERSION_MINOR}.${pekwm_VERSION_PATCH}")

# Check for C++11
check_cxx_compiler_flag(-std=c++11 HAS_CXX11)
if (HAS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else (HAS_CXX11)
  message(FATAL_ERROR "Compiler with C++11 support is required")
endif (HAS_CXX11)

# Look for dependencies
find_package(X11 REQUIRED)
find_package(Iconv REQUIRED)
find_package(Freetype)
find_package(JPEG)
find_package(PNG)

# Look for platform specific methods
check_function_exists(swprintf HAVE_SWPRINTF)
check_function_exists(setenv HAVE_SETENV)
check_function_exists(unsetenv HAVE_UNSETENV)
check_symbol_exists(timersub sys/time.h HAVE_TIMERSUB)

# Look for platform specific tools
find_program(GSED gsed /usr/bin /usr/local/bin /usr/pkg/bin)
if (GSED)
  set(SED ${GSED})
else (GSED)
  find_program(SED sed /usr/bin /usr/local/bin /usr/pkg/bin)
endif (GSED)

find_program(SH sh /usr/xpg4/bin/sh /bin/sh /usr/xpg4/bin)

# Optons
option(DEBUG "include verbose debugging code")
option(PEDANTIC "turn on strict compile-time warnings")

# Compilation
if (X11_Xshape_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} XShape")
  set(HAVE_SHAPE 1)
endif (X11_Xshape_FOUND)

if (X11_Xinerama_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} Xinerama")
  set(HAVE_XINERAMA 1)
endif (X11_Xinerama_FOUND)

if (X11_Xft_FOUND AND FREETYPE_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} Xft")
  set(HAVE_XFT 1)
endif (X11_Xft_FOUND AND FREETYPE_FOUND)

if (PNG_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} image-png")
  set(HAVE_IMAGE_PNG 1)
endif (PNG_FOUND)

if (JPEG_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} image-jpeg")
  set(HAVE_IMAGE_JPEG 1)
endif (JPEG_FOUND)

if (X11_Xpm_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} image-xpm")
  set(HAVE_IMAGE_XPM 1)
endif (X11_Xpm_FOUND)

if (X11_Xrandr_FOUND)
  set(pekwm_FEATURES "${pekwm_FEATURES} Xrandr")
  set(HAVE_XRANDR 1)
endif (X11_Xrandr_FOUND)

if (DEBUG)
  set(pekwm_FEATURES "${pekwm_FEATURES} debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif (DEBUG)

if (PEDANTIC)
  set(pekwm_FEATURES "${pekwm_FEATURES} pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")
endif (PEDANTIC)

add_definitions(-DVERSION="${pekwm_VERSION}"
  -DFEATURES="${pekwm_FEATURES}"
  -DDATADIR="${CMAKE_INSTALL_PREFIX}/share"
  -DSYSCONFDIR="${CMAKE_INSTALL_PREFIX}/etc/pekwm")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH True)

# Subdirectories
add_subdirectory(contrib)
add_subdirectory(data)
add_subdirectory(doc)
add_subdirectory(src)
