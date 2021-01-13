pekwm-0.2.0
===========

Bug fixes
---------

* **#7 new windows fail to appear on fbpanel taskbar and pager**, regression introduced in 0.1.18.

New
---


Updated
-------

**CmdDialog** no longer cache the list of available commands reducing
memory consumption and speeding up start at the cost of slower mapping
of the CmdDialog.

**SetGeometry** now support specifying size and position in % of the
screen or active head.

Examples:

```
SetGeometry 100%x50%+0+0 Current HonourStrut
SetGeometry 100x100% Screen
```

Removed
-------

