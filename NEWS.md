pekwm-0.2.1
===========

Build system
------------

* Makefile based build now install docs, support configuring the
  manpath (relative to the prefix) and create .sh files properly with
  the configured SED on Solaris
* CMAKE_INSTALL_MANDIR is now used to specify the destination of
  man pages. Use -DCMAKE_INSTALL_MANDIR=/path/to/man to override.
* Log awk, sed and sh paths during configuration (and fix a few missing
  path substitutions in the scripts)

Closed issues
-------------

* **#114 Coordinates not updating when moving windows**,
  regression introduced in 0.2.0. (Reported by caoliver)
* **#110 Tint2 panel not updated on last window close**,
  regression introduced in 0.2.0. (Reported by Maman Sulaeman)

Updated
-------

* pekwm_panel, Icon widget now has a Scale option defaulting to false.
* pekwm_theme, Detect download errors of the theme index file.

pekwm-0.2.0
===========

Build system
------------

* pekwm reverted back to using C++98 to support building on legacy
  systems.
* autotools have been removed, being replaced by CMake.
* pekwm supports a simple shell based configure script together with a
  set of mk files for building on systems without CMake.
* iconv/libiconv dependency has been removed.
* Fixed build issues on Solaris 10 and Solaris 11 with the Sun Studio
  compiler.

Closed issues
-------------

* **#7 new windows fail to appear on fbpanel taskbar and pager**,
  regression introduced in 0.1.18.
* **#14 zombines during move resize**, executing external commands
  while moving a window did not collect all child processes.
* **#22 support _NET_REQUEST_FRAME_EXTENTS**, improved EWMH compatability.
* Size specification was ignored on plain textures (except solid),
  it is now read and can cause issues on themes that have incorrectly
  specified them.
* **#75 SendToWorkspace KeepFocus**, new parameter KeepFocus added to
  SendToWorkspace.
* **#78 Icon Autoproperty**, it is now possible to set the _NET_WM_ICON
  hint using the new property Icon. It will load the image from the icon
  load path.

```
Property = "^xterm,^XTerm" {
    ApplyOn = "New"
    Icon = "term.png"
}
```
* **#79 TempRaise visual bug**, menus now refresh on expose events
  avoiding redraw issues on the selected items.
* **Correct maximized state after SetGeometry command** now corrects
  the behavior where toggle to maximize a window, execute SetGeometry
  to move it in a corner, toggle maximizing again now maximizes the
  window instead of unmaximizing it.
* **Support MSB visual** fixing black-only decorations on at least
  some Sun hardware.
* **Fix _NET_NUMBER_OF_DESKTOPS reported on sparc64**

New
---

**Crash Dialog**, if pekwm crash a pekwm_dialog will appear and prompt
the user if pekwm should be restarted or quit. This avoids the X
server to shut down.

**ImageMapped**, it is now possible map colors in images during load
using a color map from the new ColorMaps section in the theme
file. This functionality allows for creating themes using less images
and plays well with the new theme variants functionality.

Example ColorMap mapping all white pixels to red:

```
ColorMaps {
  ColorMap = "WhiteToRed" {
    Map = "#ffffff" { To = "#ff0000" }
  }
}
```

Apply it on images during loading:

```
ImageMapped WhiteToRed image.png
```

**theme variants**, using the _ThemeVariant_ option in the Files
section allows for specifying variants of themes. Theme variants are
implemented by creating separate theme files in the theme directory
named theme-VARIANT.

**pekwm_bg** created, a background setting application integrated with
pekwm themes. pekwm_bg supports all textures pekwm supports so it is
possible to set solid colors, images and the new lines texture. Themes
have been extended with a background keyword that makes pekwm set the
background when the theme is loaded, images are loaded from the
backgrounds folder inside the theme. Background loading can be
disabled in the main configuraiton file.

Example in _theme_:

```
    Background = "Image wallpaper.jpg#scaled"
```

Example in _config_:

```
    ThemeBackground = "False"
```

**pekwm_ctrl** created, simple control command for pekwm that takes
a string formatted as a single action and asks pekwm to execute it.

**pekwm_screenshot** created, simple screenshot taking application
that outputs a PNG image.

**pekwm_theme** created, theme management tool for use with the
pekwm-theme-index, enabling the user to list, search, preview,
install and uninstall themes included in the index.

**WarpPointer** action that warps the X11 pointer to the given
position. Example usage:

```
    Actions = "WarpPointer x y"
```

**CurrHeadSelector** option is now available in the Screen section of
the main configuration file. Controls how operations relative to the
current head, such as placement, select the active head. Cursor
selects the head the cursor is on, FocusedWindow considers the focused
window if any and then fall backs to the cursor position. Affected
operations include placement and position of CmdDialog, SearchDialog,
StatusWindow and focus toggle list. (#43)

**pekwm_panel** created, simpel panel application with pekwm theme
support. Displayed data, excluding a few builtin types, come from
external commands or root window properties.

Updated
-------

**CfgDeny** now support denying _ResizeInc_ making it possible to
ignore size increments for terminals and other applications. (#47)

```
CfgDeny = "ResizeInc"
```

**CmdDialog** no longer cache the list of available commands reducing
memory consumption and speeding up start at the cost of slower mapping
of the CmdDialog.

**Debug** action is included even if not compiling with DEBUG=ON. The
action allows for enabling and disabling of logging to file and
standard output. Default logging level is warning, and all messages
aimed towards end users such as theme errors are logged independent of
set level.

The initial log level can be controlled with the new --log-level
command line option.

**Exec** no longer use ``sh -c`` to run commands which will cause
incompatabilites depending on _/bin/sh_ configuration, if shell
variables have been used or the command ends with &. **ShellExec** has
been added implementing the legacy behaviour.

**SetGeometry** now support specifying size and position in % of the
screen or active head.

Examples:

```
SetGeometry 100%x50%+0+0 Current HonourStrut
SetGeometry 100x100% Screen
```

**Fullscreen Focus** changes where fullscreen windows (using
FullscreenAbove) are temporarily lowered when a non fullscreen window
is raised to enable interactions with the client.

**Icon load path** now include the current theme path icons folder.

**$_PEKWM_CONFIG_PATH** is now set to the path where the configuration
file was read from.

Removed
-------

**PDecor** section in themes is no longer required, all Decor sections
in the top-level will be used if no PDecor section is found.

**InputDialog** is no longer possible to use as the decor name for
CmdDialog decorations in themes.
