pekwm-0.2.0
===========

Bug fixes
---------

* **#7 new windows fail to appear on fbpanel taskbar and pager**,
  regression introduced in 0.1.18.

New
---

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

Updated
-------

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

Removed
-------

