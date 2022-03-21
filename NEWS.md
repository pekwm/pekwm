pekwm-0.3.0
===========

Closed issues
-------------

 * #126, Improved XDG config directory support by making use of
   $PEKWM_CONFIG_PATH instead of $HOME in scripts and copy configuration
   files to dirname of provided configuration file.
 * #129 Centered placement model. Simple placement model where the
   window is placed centered on the screen. Struts (panels etc) are
   taken into consideration.

New
---

* **FocusWithSelector string (string...)** action that can be used to
  explicitly try to focus a window. The currently available selectors are:
  _pointer_, _workspacelastofocused_, _top_ and _root_.
* **--standalone** option to _pekwm_wm_, convenience to improve the debugging
  experience.
* **FontDefaultX11** and **FontCharsetOverride** options added to the _Screen_
  section of the main configuration file. Controls the default font type and
  override of charset in font strings.
* **Setenv** action added, making it possible to update the environment pekwm
  use when executing applications without restarting pekwm.
* **CenteredOnParent** placement model replacing TransientOnParent re-added.

### pekwm_panel

* **Icon widget now can Exec** command on click, configure by setting
  _Exec_ to the command to be executed in the Icon configuration.

Updated
-------

* **GotoWorkspace** now supports a second argument being a boolean. If set
  to False (default is True) pekwm will not try to focus a window on
  workspace switching. To be used in combination with other focus actions.

Removed
-------

* C++ setlocale warning during startup is no longer output, silent fallback
  to setlocale()
* Remove TransientOnParent option, replaced by CenteredOnParent placement
  model.

### pekwm_bg

* Removed warning about being unable to send kill signal if atom is missing.
