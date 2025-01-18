pekwm-0.3.3
===========

### pekwm_dialog

  * Fix dialog redraw issue making text in dialog not visible.

pekwm-0.3.2
===========

Closed issues
-------------

  * Disable SA_RESTART for SIGALRM, fixes WorkspaceIndicator getting stuck
    on OpenBSD.

### pekwm_panel

  * Fix crash on client list updates, triggered by XRandr updates

pekwm-0.3.1
===========

Closed issues
-------------

  * #170, Reduce flickering when placing new windows.
  * #175, GotoItem N now skips separators and non-visible items.
  * #163, ButtonRelase actions on the Client working again.
  * #161, GotoItem N regression caused it to behave as NextItem.
  * Use X double buffer extension in pekwm_panel and pekwm_dialog to avoid
    flickering on redraws.
  * X11 font name detection would fail if any font options was specified,
    such as alignment.

### pekwm_panel

  * Setup PEKWM_CONFIG_PATH and PEKWM_CONFIG_FILE in the environment of
    pekwm_panel as it is used for icon loading, fixes icon loading issues
    when pekwm_panel started before pekwm.
  * Update ClientList on panel start, fixes blank client list until window
    focus is changed.
  * Fix text rendering after receiving XRANDR events when using Pango Cairo
    fonts.

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

* Font type detection. If a font type is missing, X11 will be used if the name
  is in the form -misc-fixed-*-*-*-*-12-*-*-*-*-*-*-*. Else, depending on what
  fonts pekwm supports, Pango, Xft or Xmb/X11 font will be used matching the
  provided font name and properties. Font information is provided in
  Xft/fontconfig format.
* Pango font support, types PangoCairo and PangoXft using Cairo and Xft
  backends respectively.
* ${NAME} style variables now supported in the configuration, convenient
  when using resource names.
* $@ and $& variables available in configuration files. $@ATOM_NAME
  gets String atom values. $&resource gets String resource values from
  RESOURCE_MANAGER resources.
* &resource colors available in themes (not variables). When used, the
  theme will re-load when resources are updated.
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
* Scripts setting in the Files section can now be used to add a path to
  the PATH when searching for COMMANDs in configuration files.

### pekwm_panel

* **Icon widget now can Exec** command on click, configure by setting
  _Exec_ to the command to be executed in the Icon configuration.
* **Systray widget** added with support for housing systray icons.
  Example configuration:

```
    Systray {
      Size = "Required"
    }
```

Updated
-------

* **GotoWorkspace** now supports a second argument being a boolean. If set
  to False (default is True) pekwm will not try to focus a window on
  workspace switching. To be used in combination with other focus actions.
* Variables in sections are now expandend, making the follow configuration
  use the value of $VAR and not literal $VAR:

```
    Section = "$VAR" { }
```

Removed
-------

* C++ setlocale warning during startup is no longer output, silent fallback
  to setlocale()
* Remove TransientOnParent option, replaced by CenteredOnParent placement
  model.
* Font specifications no longer support specifying the type of the font at
  the end. (XFT#... works, ...#XFT no longer does)

### pekwm_bg

* Removed warning about being unable to send kill signal if atom is missing.
