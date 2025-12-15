pekwm-0.4.2
===========

Closed issues
-------------

* #205, SysTray icons not displaying properly due to missing configure notify.
* Fix out of bounds vector access in pekwm_sys TimeOfDay command and stdin
  reading in interactive mode.

pekwm-0.4.1
===========

Closed issues
-------------

* #199, unintended N: prefix on _NET_WORKSPACE_NAMES
* #203, MoveToEdge and FillEdge actions failed to get the current head
  always moving the window to the first head.

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
* New Screen option `WarpPointerOn`, if set the pointer will be warped to the
  `New` clients or when a client is focused using `FocusChange`. Empty string
  to disable. (Close #186)
* Themes can now have an `XResources` section specifying per theme X11
  resources.
* `WorkspacesBackAndForth` option in the `Screen` section of the configuration
  added. When enabled, GotoWorkspace Num to the currently active workspace
  will go to the previously active workspace. Allows for quick and easy switch
  between two workspaces. Default is enabled.
* `DetachSplitHorz` and `DetachSplitVert` actions detaching a client and
  dividing the space between the existing frame and newly created one from
  the detach action. Takes one argument, percent between 5-95 that the client
  being detached should occupy of the current frames size.
* #192, add `TextBackground` to themes supporting a separate texture under
  the text render on titles. Font padding is included in the width when
  rendering the texture.
* #194, add Sys XSetInt and XSetColor for setting integer and color XSETTINGS.
* #194, add Sys Dpi command for dynamic reconfiguration of the current Dpi
  XSETTING and X resource.
* #194, add WmSet Scale for dynamic override of the Scale setting.
* Add WorkspaceName to WmSet for overriding workspace names.

Updated
-------

* Keys updated to use $MOVERESIZE_INCREMENT (set in default vars file) for
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
* Menu items can now have shortcuts by prefixing the letter of the item with _.
  Pressing that letter when the menu has focus will select and execute that
  menu item.

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
* Add noicon option to ClientList widget disable rendering of client icons.

## pekwm_screenshot

* Add --wait option to screenshot for delaying the screenshot capture.

## pekwm_sys

* New process acting as an XSETTINGS daemon, tracks daytime changes and updates
  X resources/XSETTINGS on daytime change in order to support automatic
  light/dark mode changing.
* Controlled from pekwm with the `Sys` command that supports the following
  sub-commands:
  * `TimeOfDay`, override time of day. Can be `Dawn`, `Day`, `Dusk`, `Night`
    and `Auto`.
  * `XSet`, set XSETTING String value.
  * `XSave`, save all XSETTING values to `~/.pekwm/xsettings.save`. Settings
    will be loaded on start.
