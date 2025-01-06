pekwm-0.4.0
===========

Closed issues
-------------

* #147, picom fade out visual artifacts.
* #21, support `_NET_RESTACK_WINDOW`.

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

Updated
-------

* keys updated to use $MOVERESIZE_INCREMENT (set in default vars file) for
  actions in the MoveResize section. Default bumped from 10 to 15.

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
