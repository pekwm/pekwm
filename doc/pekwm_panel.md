# pekwm_panel overview

pekwm_panel is a simple panel application bundled with pekwm that aims
to have a small core and be extensible using external scripts.

pekwm_panel integrates with the pekwm themes and aims to support pekwm
specific window hints.

## Configuration

The default configuration is placed in _~/.pekwm/panel_ and configures
the _widgets_ displayed, panel _placement_ and external _commands_.

**Placement** _Top_ or _Bottom_

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

* WorkspaceNumber, Size = "Required"
* ExternalData, Size = "Pixels 300"
* ClientLIst, Size = "*"

For simplicity space for the separators is excluded.

```
                          800px
----------------------------------------------------------------------------
|  [1]  | [ExternalData] |                 [ClientList]                    |
----------------------------------------------------------------------------
  30px        300px                           470px
```

**Interval** update interval in seconds

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
}
```

#### Widget: ClientList

Dispay a list of clients on the current workspace together with the
client icon if _NET_WM_ICON is set.

Widget configuration:

```
ClientList {
  Size = "*"
}
```

#### Widget: DateTime

Display date and time using a _strftime(3)_ format string.

Widget configuration:

```
DateTime = "%Y-%m-%d %H:%M" {
  Size = "Required"
  Interval = "60"
}
```

#### Widget: ExternalData

Display external data from a given field extracted from an external
command. The widget is updated whenever the data field is updated.

It is recommended to use _TextWidth_ for the _Size_ parameter to
handle font size differences between themes.

Widget configuration:

```
ExternalData = "field" {
  Size = "TextWidth _value_"
}
```

#### Widget: WorkspaceNumber

Widget displaying the current workspace number.

Widget configuration:

```
WorkspaceNumber {
  Size = "Required"
}
```

### Commands

The **Commands** section of the panel configuration includes
configuration for external commands which pekwm_panel will run at given
intervals to collect data displayed by the _ExternalData_ and _Bar_
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
  ExternalData = "date" {
    Size = "TextWidth _Ddd Mmm 00 00:00:00 ZZZ YYYY_"
  }
}
```
