pekwm-0.3.0
===========

Closed issues
-------------

New
---

* **FocusWithSelector string (string...)** action that can be used to
  explicitly try to focus a window. The currently available selectors are:
  _pointer_, _workspacelastofocused_, _top_ and _root_.
* **--standalone** option to _pekwm_wm_, convenience to improve the debugging
  experience.

Updated
-------

* **GotoWorkspace** now supports a second argument being a boolean. If set
  to False (default is True) pekwm will not try to focus a window on
  workspace switching. To be used in combination with other focus actions.

Removed
-------

