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

Updated
-------

**CmdDialog** no longer cache the list of available commands reducing
memory consumption and speeding up start at the cost of slower mapping
of the CmdDialog.

**Debug** action is included even if not compiling with DEBUG=ON. The
action allows for enabling and disabling of logging to file and
standard output.

**SetGeometry** now support specifying size and position in % of the
screen or active head.

Examples:

```
SetGeometry 100%x50%+0+0 Current HonourStrut
SetGeometry 100x100% Screen
```

Removed
-------

