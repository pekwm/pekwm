pekwm-0.4.0
===========

Closed issues
-------------

* #147, picom fade out visual artifacts.

New
---

* Use pledge on pekwm_* commands on systems where it is available (OpenBSD)
* Add Debug section to config, making it possible to enable debug logging
  at startup. Log file is no longer truncated when it is opened.

Updated
-------

Removed
-------

## pekwm_ctrl

* -g (--xrm-get) and -s (--xrm-set) commands for reading and writing
  the Xresources.
