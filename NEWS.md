pekwm-0.4.0
===========

Closed issues
-------------

* #147, picom fade out visual artifacts.
* #21, support `_NET_RESTACK_WINDOW`.
* Configuration default values was not picked up properly if the node was
  missing from the configuration.
* `_NET_WM_STATE` ClientMessage with action unset should no longer cause the
  property to be set.
* Always remove Window from the save set when deleting a Client. Fix issues
  with client modals ending up as empty black windows on restart.

New
---

* Use pledge on pekwm_* commands on systems where it is available (OpenBSD)
* Add Debug section to config, making it possible to enable debug logging
  at startup. Log file is no longer truncated when it is opened.
* Add Theme section to config. Currently has a single element,
  BackgroundOverride for overriding the background set in the theme file.
* Add `OnCloseFocusStacking` (boolean, default false) and
  `OnCloseFocusRaise` (Always|Never|IfCovered, default Always) settings under
  the Screen section.
  `OnCloseFocusStacking`, if set to false, use the stacking order instead of
  the MRU list to find a client when the focused object is closed.
  `OnCloseFocusRaise`, controls wheter the window focused is raised. IfCovered
  will raise the window if it is mostly covered by other windows.
* Add auto property `Placement` for overriding the placement model for specific
  clients.
* Add autoproperties_clientrules file, not installed in ~/.pekwm, that contain
  client specific rules to help improve compatibility with pekwm. Created as a
  separate file to avoid users having to update their configuration file on
  upgrades when rules get added or removed.
* Auto theme variant mode, reads the _PEKWM_THEME_VARIANT property on the root
  window and selects the theme variant using that value. The theme will
  automatically reload whenever the property changes.
* pekwm_audio_ctrl.sh script for controlling playback/volume with default
  keybindings for XF86Audio* keys.
* New Screen option `Scale`, affects how themes and other dimensions given in
  the configuration is processed. Default is 1.0, treated as is, for high
  resolution displays set to 2.0 for 2x scaled images, padding etc.
* Add action `FillEdge` making a window fill given edge occupying N% of the
  "other" direction of the screen. `FillEdge LeftEdge 40` will make the window
  use the full left edge being 40% of the screen width wide.
  `FillEdge` on the opposite edge will make the window return to the size it
  had before the initial `FillEdge`.

Updated
-------

* keys updated to use $MOVERESIZE_INCREMENT (set in default vars file) for
  actions in the MoveResize section. Default bumped from 10 to 15.
* Configuration file keys have been updated to support quoted syntax, allowing
  newlines and other special characters.

```
Section {
    "Special Key" = "Value"
}
```

* Variable expansion failures in the configuration are now logged with
  source:line:pos context information.
* Command/Search dialog now react to mouse clicks for repositioning the cursor.
* ClearToCursor (Mod1 + Backspace) new InputDialog keybinding for removing all
  input up until the current cursor position.
* Asymetric tabs now grow up until minimum width.

Removed
-------

## pekwm_ctrl

* -g (--xrm-get) and -s (--xrm-set) commands for reading and writing
  the Xresources.

## pekwm_panel

* Added pekwm_panel_battery.sh script for retreiving battery charge level in a
  platform independent manner. Sets battery (0-100) and battery_status
  (ac|battery) variables.
* Fix icon rendering as garbage + cache of scaled icons for improved
  performance.
* Add Assign option to Command section, if set, assign complete lines of output
  to the variable name set in Assign instead of parsing the output.
* Add Text to Bar widget, allows to render text inside the bar area.

## pekwm_screenshot

* Add --wait option to screenshot for delaying the screenshot capture.

## pekwm_sys

* New process acting as an XSETTINGS daemon, tracks daytime changes and updates
  X resources/XSETTINGS on daytime change in order to support automatic
  light/dark mode changing.
