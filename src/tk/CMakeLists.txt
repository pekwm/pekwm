cmake_minimum_required(VERSION 3.10)

set(tk_SOURCES
    Action.cc
    CfgUtil.cc
    Color.cc
    ColorPalette.cc
    FontHandler.cc
    ImageHandler.cc
    PFont.cc
    PFontX.cc
    PFontX11.cc
    PFontXmb.cc
    PImage.cc
    PImageIcon.cc
    PTexture.cc
    PTexturePlain.cc
    PWinObj.cc
    PXftColor.cc
    Render.cc
    TextureHandler.cc
    Theme.cc
    ThemeUtil.cc
    TkButton.cc
    TkButtonRow.cc
    TkText.cc
    X11App.cc
    X11Util.cc)

if (PEKWM_HAVE_XFT)
	set(tk_SOURCES ${tk_SOURCES} PFontXft.cc)
endif (PEKWM_HAVE_XFT)

if (PEKWM_HAVE_PANGO_CAIRO OR PEKWM_HAVE_PANGO_XFT)
	set(tk_SOURCES ${tk_SOURCES} PFontPango.cc)
endif (PEKWM_HAVE_PANGO_CAIRO OR PEKWM_HAVE_PANGO_XFT)

if (PEKWM_HAVE_PANGO_CAIRO)
	set(tk_SOURCES ${tk_SOURCES} PFontPangoCairo.cc)
endif (PEKWM_HAVE_PANGO_CAIRO)

if (PEKWM_HAVE_PANGO_XFT)
	set(tk_SOURCES ${tk_SOURCES} PFontPangoXft.cc)
endif (PEKWM_HAVE_PANGO_XFT)

if (ENABLE_IMAGE_JPEG AND JPEG_FOUND)
	set(tk_SOURCES ${tk_SOURCES} PImageLoaderJpeg.cc)
endif (ENABLE_IMAGE_JPEG AND JPEG_FOUND)

if (ENABLE_IMAGE_PNG AND PNG_FOUND)
	set(tk_SOURCES ${tk_SOURCES} PImageLoaderPng.cc)
endif (ENABLE_IMAGE_PNG AND PNG_FOUND)

if (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)
	set(tk_SOURCES ${tk_SOURCES} PImageLoaderXpm.cc)
endif (ENABLE_IMAGE_XPM AND X11_Xpm_FOUND)

add_library(tk STATIC ${tk_SOURCES})
target_include_directories(tk PUBLIC
			   ${PROJECT_BINARY_DIR}/src/tk
			   ${common_INCLUDE_DIRS})
target_compile_definitions(tk PUBLIC PEKWM_SH="${SH}")
