< Prev
\- [Up](README.md)
\- Next >

# pekwm_panel overview

pekwm_panel is a simple panel application bundled with pekwm that aims
to have a small core and be extensible using external scripts.

pekwm_panel integrates with the pekwm themes and aims to support pekwm
specific window hints.

## Configuration

The default configuration is placed in _~/.pekwm/panel_ and configures
the _widgets_ displayed, panel _placement_ and external _commands_.

### Panel

The panel section contains configuration for the panel itself.

**Placement**, place panel at the _Top_ or _Bottom_ of the head.

**Head**, if present place the panel on the given head (number)
instead of spanning all heads.

### Widgets

All widgets share two common configuration parameters.

#### Common configuration

**Size**

The _Size_ parameter controls how much of the panel each widget is
going to consume and can be specified in one of the units below.

| Name      | Type    | Description                                                             |
|-----------|---------|-------------------------------------------------------------------------|
| Pixels    | Integer | Screen pixels, use only for fixed size widgets                          |
| Percent   | 1-100   | Percent of total panel width                                            |
| Required  | NA      | Calculated required width using the current theme                       |
| TextWidth | String  | Width of the provided string using the current theme                    |
| *         | NA      | Use rest of available space, divided equally between all widgets with * |

The configuration below would distribute the available
800 pixels as follows:

* DateTimeWidget, Size = "Required"
* Text, Size = "Pixels 250"
* ClientList, Size = "*"

For simplicity space for the separators is excluded.

```
                          800px
----------------------------------------------------------------------------
|  [1]  |    [Text]      |                 [ClientList]                    |
----------------------------------------------------------------------------
  100px       250px                           450px
```

**Interval**

Update interval in seconds

**If**

Condition controlling whether the widget should be displayed or not. The
condition can reference variable values and will be re-evaluated in case the
value is changed.

The operators: `=`, `!=`, `<` and `>` are supported. `<` and `>` require
numeric values for the comparison to be true.

The empty string and `true` always evaluate to true.

Example:

```
Bar = "battery" {
    If = "%battery_count > 0"
	...
}
```

#### Widget: Bar

Display a partially filled bar, where the fill percent is read from
a field extracted from an external command.

The fill percent should be a numeric value between 0.0 and 100.0,
values below and and above will be set to 0.0 and 100.0 respectively.

If it is not possible to parse the value it will default to 0.0.

Example field output usable with the bar which will render the
bar at 50%:

```
fill-percent 50
```

Widget configuration:

```
Bar = "field" {
  Size = "Pixels 32"
  Colors {
    Percent = "80" { Color = "#00ff00" }
    Percent = "40" { Color = "#ffff00" }
    Percent = "10" { Color = "#ff0000" }
  }
}
```

#### Widget: ClientList

Display a list of clients on the current workspace together with an icon, read
from the `_NET_WM_ICON` property, if one is set.

Widget configuration:

```
ClientList = "separator" {
  Size = "*"
}
```

* Separator, if set in the argument list, a separator is drawn in-between
  clients in the list of clients.

#### Widget: Systray

Display system tray icons provided by programs like NetworkManager, VLC, etc.

Widget configuration:

```
Systray {
  Size = "Required"
}
```

The size of the widget will adapt to the number of icons in the tray.

#### Widget: DateTime

Display date and time using a _strftime(3)_ format string.

Widget configuration:

```
DateTime = "%Y-%m-%d %H:%M" {
  Size = "Required"
  Interval = "60"
}
```

#### Widget: Icon

Widget displaying an icon. The icon can be configured to update whenever
a given external data field is updated.

```
Icon = "battery-icon-status" {
  Size = "Pixel 32"
  Icon = "battery.png"
  # If True, scale image square to panel height
  Scale = "False"
}
```

NOTE: The widget expects all icons set by the external data field to
have the same size.

The Icon widget supports executing a command when it is clicked. To enable
command execution add an Exec statement, the format of the command will be
expanded the same way as the _Text_ widget before being executed.

#### Widget: Text

Display formatted text. The format string can reference environment
variables, external command data and specific window manager state
variables.

The widget is updated whenever any field in the format string is
updated.

It is recommended to use _TextWidth_ for the _Size_ parameter to
handle font size differences between themes.

All format string variables start with **%**, the second character
determine which type of data is referenced.

* **%_**, reference environment variables. Example: __%_USER__.
* **%:**, reference window manager state, see table below.
* **%**, reference external command data.


| Variable           | Description                  |
|--------------------|------------------------------|
| :CLIENT_NAME:      | Name of the active client    |
| :WORKSPACE_NAME:   | Name of the active workspace |
| :WORKSPACE_NUMBER: | Active workspace number      |


Widget configuration:

```
Text = "format string" {
  Size = "TextWidth _value_"
}
```

#### Transform

The **Text** and **Icon** widgets support transformation of the value
of the field the widget is using. For the **Text** widget the
formatted string is transformed.

The transformation can be used if the value of pre-defined fields,
such as X11 atoms, requires some change before it can be used.

Transform takes the same form as TitleRules do in pekwm, and is
configured using the **Transform** keyword.

Example using the _XKB_RULES_NAMES atom to display a keyboard language
indicator in the panel. The _XKB_RULES_NAMES is a multi value string
looking something like "evdev,pc105,us,,".

The below example shows the use of a transform, to display kbd-en.png,
kbd-se.png etc. icon depending on the current keyboard layout.

```
Icon = "ATOM__XKB_RULES_NAMES" {
  Icon = "kbd.png"
  Transform = "/^[^,]*,[^,]*,([^,]*),.*/\\1/"
}
```

### Commands

The **Commands** section of the panel configuration includes
configuration for external commands which pekwm_panel will run at
given intervals to collect data displayed by the _Text_ and _Bar_
widgets.

The commands run shall output data in the given format:

_field_[space]_value_

Example:

```
field1 value one
field2 value two
```
### Long running commands

pekwm_panel parses data as it is output by the command and it is thus
possible to have a single long-running command providing data for all
widgets requiring external data.

It is recommended to use long-running commands if frequent updates of
the displayed data is required.

A simple example displaying the current time every second without
using the _DateTime_ widget could look this:

**date.sh**

```
#!/bin/sh

while `/usr/bin/true`; do
    echo date `date`
    sleep 1
done
```

**panel**

```
Commands {
  Command = "/path/to/date.sh" {
    # time to wait between runs if date.sh crash
    Interval = "3600"
  }
}

Widgets {
  Text = "date" {
    Size = "TextWidth _Ddd Mmm 00 00:00:00 ZZZ YYYY_"
  }
}
```
