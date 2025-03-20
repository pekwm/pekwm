[< Previous (Basic Usage)](basic-usage.md)
\- [Up](README.md)
\- [(Autoproperties) Next >](autoproperties.md)

***

Configuration
=============

The configuration part of this documentation is focused on providing
complete documentation for all config files located in your ~/.pekwm
directory.

**Table of Contents**

1. [The pekwm Common Syntax for Config Files](#basic-syntax)
1. [The main config file](#the-main-config-file)
1. [Configuring the menus](#configuring-the-menus)
1. [Keyboard and Mouse Configuration](#keyboard-and-mouse-configuration)
1. [The pekwm start file](#the-pekwm-start-file)

The pekwm Common Syntax for Config Files
----------------------------------------

### Basic Syntax

All pekwm config files (with the exception of the start file- see
[start file](#the-pekwm-start-file) ) follow a common syntax for options.

```
# comment
// another comment
/*
	yet another comment
*/

$VARIABLE = "Value"
$_ENVIRONMENT_VARIABLE = "Value"
INCLUDE = "another_configuration.cfg"
COMMAND = "program to execute and add the valid config syntax it outputs here"

# Normal format
Section = "Name" {
	Event = "Param" {
		Actions = "action parameter; action parameter; $VAR $_VARIABLE"
	}
}

// Compressed format
Section = "Name" { Event = "Param" { Actions = "action parameters; action parameters; $VAR $_VARIABLE" } }
```

One can usually modify the spacing and line breaks, but this is the
"Correct" format, so the documentation will try to stick to it.

Events can be combined into the same line by issuing a semicolon
between them. Actions can be combined into the same user action by
issuing a semicolon between them. You can use an INCLUDE anywhere in
the file.

pekwm has a vars file to set common variables between config
files. Variables are defined in vars and the file is INCLUDEd from the
configuration files.

Comments are allowed in all config files, by starting a comment line
with # or //, or enclosing the comments inside /\* and \*/.

### Template Syntax

The pekwm configuration parser supports the use of templates to reduce
typing. Template support is currently available in autoproperties and
theme configuration files. Template configuration is not fully
compatible with the non-template syntax and thus requires activation
to not break backwards compatibility. To enable template parsing, start
the configuration file with the following:

```
Require {
	Templates = "True"
}
```

Defining templates is just like creating an ordinary section; however it uses
the special name Define. Below is an example defining a template named
NAME:

```
Define = "NAME" {
  Section = "Sub" {
    ValueSub = "Sub Value"
  }
  Value = "Value"
}
```

The above defined template can later be used by using the magic
character @. The below example shows usage of the template NAME in a
two sections named Name and NameOverride overriding one of the
template values:

```
Section = "Name" {
  @NAME
}
Section = "NameOverride" {
  @NAME
  Value = "Overridden value"
}
```

The above example is equivalent of writing the following:

```
Section = "Name" {
  Section = "Sub" {
    ValueSub = "Sub Value"
  }
  Value = "Value"
}
Section = "Name" {
  Section = "Sub" {
    ValueSub = "Sub Value"
  }
  Value = "Overridden"
}
```

### Variables In pekwm Config Files

pekwm config enables you to use both internal to pekwm variables, as
well as global system variables. Internal variables are prefixed with
a **$**, global variables with **$\_**.

```
# examples of how to set both type of variables
$INTERNAL = "this is an internal variable"
$_GLOBAL = "this is a global variable"

# examples of how to read both type of variables
RootMenu = "Menu" {
	Entry = "$_GLOBAL" { Actions = "xmessage $INTERNAL" }
}
```

There is one special global variable pekwm handles. It is called
$\_PEKWM\_CONFIG\_FILE. This global variable is read when pekwm
starts, and it's contents will be used as the default config file. It
will also be updated to point to the currently active config file if
needed.

A few other variables defined by pekwm:

- $\_PEKWM\_ETC\_PATH points to the system configuration directory,
  set at compile time. This is usually /etc.
- $\_PEKWM\_SCRIPT\_PATH points to the "scripts" directory of pekwm,
  e.g. /usr/share/pekwm/scripts.
- $\_PEKWM\_THEME\_PATH points to the "themes" directory of pekwm,
  e.g. /usr/share/pekwm/themes.
- $\_PEKWM\_CONFIG\_PATH contains the directory path of
  $\_PEKWM\_CONFIG\_FILE

Variables can probably be defined almost anywhere, but it's probably a
better idea to place them at the top of the file, outside of any
sections.

The main config file
--------------------

The main config file is where all the base config stuff goes.

### Basic Config

As previously indicated, the config file follows the rules defined in
[Common Syntax](#common-syntax).

Here's an example ~/.pekwm/config file:

```
Files {
	Keys = "~/.pekwm/keys"
	Mouse = "~/.pekwm/mouse"
	Menu = "~/.pekwm/menu"
	Start = "~/.pekwm/start"
	AutoProps = "~/.pekwm/autoproperties"
	Theme = "~/.pekwm/themes/default"
	Icons = "~/.pekwm/icons/"
}

MoveResize {
	EdgeAttract = "10"
	EdgeResist = "10"
	WindowAttract = "5"
	WindowResist = "5"
	OpaqueMove = "True"
	OpaqueResize = "False"
}

Screen {
	Workspaces = "4"
	WorkspacesPerRow = "2"
	WorkspaceNames = "Main;Web;E-mail;Music"
	ShowFrameList = "True"
	ShowStatusWindow = "True"
	ShowStatusWindowCenteredOnRoot = "False"
	ShowClientID = "False"
	ShowWorkspaceIndicator = "500"
	FocusNew = "True"
	PlaceNew = "True"

	TrimTitle = "..."
	FullscreenAbove = "True"
	FullscreenDetect = "True"
	HonourRandr = "True"
	HonourAspectRatio = "True"
	EdgeSize = "1 1 1 1"
	EdgeIndent = "False"
	DoubleClickTime = "250"

	CurrHeadSelector = "Cursor"

	Placement {
		Model = "Smart"
		Smart {
			Row = "False"
			TopToBottom = "True"
			LeftToRight = "True"
			OffsetX = "0"
			OffsetY = "0"
		}
	}

	UniqueNames {
		SetUnique = "True"
		Pre = " #"
		Post = ""
	}
}

Menu {
	DisplayIcons = "True"

	Icons = "DEFAULT" {
		Minimum = "16x16"
		Maximum = "16x16"
	}

	Select = "Motion"
	Enter = "Motion ButtonPress"
	Exec = "ButtonRelease"
}

CmdDialog {
	HistoryUnique = "True"
	HistorySize = "1024"
	HistoryFile = "~/.pekwm/history"
	HistorySaveInterval = "16"
}

Harbour {
	OnTop = "True"
	MaximizeOver = "False"
	Placement = "Right"
	Orientation = "TopToBottom"
	Head = "0"

	DockApp {
		SideMin = "64"
		SideMax = "0"
	}
}
```

### Config File Keywords

Here's a table showing the different elements that can be used in your
config file. Remember that 'boolean' means 'true' or 'false' and that
all values should be placed inside quotes.

**Config File Elements under the Files-section:**

| Keyword   | Type   | Description                                                            |
|-----------|--------|------------------------------------------------------------------------|
| Keys      | string | The location of the keys file, such as ~/.pekwm/keys                   |
| Menu      | string | The location of the menu file, such as ~/.pekwm/menu                   |
| Start     | string | The location of the start file, such as ~/.pekwm/start                 |
| AutoProps | string | The location of the autoprops file, such as ~/.pekwm/autoproperties    |
| Theme     | string | The location of the Theme directory, such as ~/.pekwm/themes/themename |
| Icons     | string | The location of the Icons directory, such as ~/.pekwm/icons            |

**Config File Elements under the MoveResize-section:**

| Keyword       | Type    | Description                                                                                     |
|---------------|---------|-------------------------------------------------------------------------------------------------|
| EdgeAttract   | int     | The distance from screen edge required for the window to snap against it in pixels.             |
| EdgeResist    | int     | The distance from screen edge required for the window moving to start being resisted in pixels. |
| WindowAttract | int     | The distance from other clients that a window will snap against them to in pixels.              |
| WindowResist  | int     | The distance from other clients that a window movement will start being resisted.               |
| OpaqueMove    | boolean | If true, turns on opaque Moving                                                                 |
| OpaqueResize  | boolean | If true, turns on opaque Resizing                                                               |

**Config File Elements under the Screen-section:**

| Keyword                        | Type            | Description                                                                                                                                                               |
|--------------------------------|-----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| ThemeBackground                | boolean         | Set to False to disable background setting from themes. Default True.                                                                                                     |
| Workspaces                     | int             | Number of workspaces enabled.                                                                                                                                             |
| WorkspacesPerRow               | int             | Number of workspaces on each row. If set to 0, all workspaces are placed on a single row.                                                                                          |
| WorkspaceNames                 | string          | List of names for workspaces separated by semicolon.                                                                                                                      |
| WarpPointerOn                  | string          | Space separated list. New to warp pointer on new clients. FocusChange to warp pointer when client focus changes. Empty to disable.
| ShowFrameList                  | boolean         | Controls whether a list of all available frames on the workspace is displayed during the NextFrame/PrevFrame actions.                                                     |
| ShowStatusWindow               | boolean         | Controls whether a size/position info window is shown when moving or resizing windows.                                                                                    |
| ShowStatusWindowCenteredOnRoot | boolean         | Controls whether a size/position info window is shown centered on the current head or the current window.                                                                 |
| ShowClientID                   | boolean         | Should Client IDs be displayed in window titles.                                                                                                                          |
| ShowWorkspaceIndicator         | int             | Show WorkspaceIndicator for N milliseconds. If set to 0, the WorkspaceIndicator is disabled.                                                                            |
| WorkspaceIndicatorScale        | int             | Changes the size of the WorkspaceIndicator, higher value means smaller size.                                                                                              |
| WorkspaceIndicatorOpacity      | int             | Sets the opacity/transparency of the WorkspaceIndicator. A value of 100 means completely opaque, while 0 stands for completely transparent.                               |
| FocusNew                       | boolean         | Toggles if new windows should be focused when they pop up.                                                                                                                |
| FocusNewChild                  | boolean         | Toggles if new transient windows should be focused when they pop up if the window they are transient for is focused.                                                      |
| OnCloseFocusStacking           | boolean         | Toggles if window focused due to another window being closed is selected based on stacking (true) or most recently used (false). Default false.
| OnCloseFocusRaise              | string          | Sets raise behaviour of window focused due to another window being closed. Always, Never, IfCovered. Default Always.
| PlaceNew                       | boolean         | Toggles if new windows should be placed using the rules found in the Placement subsection, or just opened on the top left corner of your screen.                          |
| ReportAllClients               | boolean         | Toggles if all clients in a frame or only the active one should be reported and thus displayed in pager applications etc.                                                 |
| TrimTitle                      | string          | This string contains what pekwm uses to trim down overlong window titles. If it's empty, no trimming down is performed at all.                                            |
| FullscreenAbove                | boolean         | Toggles re-stacking of windows when going to and from fullscreen mode. Windows are re-stacked to the top of all windows when going to fullscreen and to the top of their layer when being restored from fullscreen. However, if another window is raised it will move fullscreen windows back to its layer. Next time a fullscreen window is raised, it's back to be on top of all windows again. |
| FullscreenDetect               | boolean         | Toggles detection of broken fullscreen requests setting clients to fullscreen mode when requesting to be the size of the screen. Default true.                            |
| HonourRandr                    | boolean         | Toggles reading of XRANDR information, this can be disabled if the display driver gives both Xinerama and Randr information and only of the two is correct. Default true. |
| HonourAspectRatio              | boolean         | Toggles if pekwm respects the aspect ratio of clients (XSizeHints). Default true.                                                                                         |
| EdgeSize                       | int int int int | How many pixels from the edge of the screen should screen edges be. Parameters correspond to the following edges: top bottom left right. A value of 0 disables edges.     |
| EdgeIndent                     | boolean         | Toggles if the screen edge should be reserved space.                                                                                                                      |
| DoubleClickTime                | int             | Time, in milliseconds, between clicks to be counted as a doubleclick.                                                                                                     |
| CurrHeadSelector               | string          | Controls how operations relative to the current head, such as placement, select the active head. Cursor selects the head the cursor is on, FocusedWindow considers the focused window if any and then fall backs to the cursor position. Affected operations include placement and position of CmdDialog, SearchDialog, StatusWindow and focus toggle list. |
| FontDefaultX11                 | boolean         | If true, default font type is X11 else XMB if no font type is specified in font string.                                                                                   |
| FontCharsetOverride            | string          | If set, overrides the charset of X11/XMB fonts with the specified string. Should be in format iso8859-1                                                                   |
| Scale                          | float           | If set, UI elements will be scaled by the given factor. If the factor is 2.0, 3.0 etc the resulting pixels will be square and without any visible artifacts.              |

>  NOTE: A Composite Manager needs to be running for opacity options to take effect.

**Config File Elements under the Placement-subsection of the Screen-section:**

| Keyword             | Type    | Description                                                                                                                       |
|---------------------|---------|-----------------------------------------------------------------------------------------------------------------------------------|
| Model               | string  | Default placement model, one of the below Placement Models.                                                                       |
Placement Models:

* _Smart_, Tries to place windows where no other window is present
* _Centered_, Places the window centered on the current head.
* _MouseCentered_, Places the center of the window underneath the current mouse pointer position
* _MouseTopLeft_, Places the top-left corner of the window under the pointer
* _MouseNotUnder_, Places windows on screen corners avoiding the current mouse cursor position.
* _CenteredOnParent_, Places transient windows at center of their parent window.

**Config File Elements under the Smart-subsection of the Placement-subsection:**

| Keyword     | Type    | Description                                                                                 |
|-------------|---------|---------------------------------------------------------------------------------------------|
| Row         | boolean | Whether to use row or column placement, if true, uses row.                                  |
| TopToBottom | boolean | If false, the window is placed starting from the bottom.                                    |
| LeftToRight | boolean | If false, the window is placed starting from the right.                                     |
| OffsetX     | int     | Pixels to leave between new and old windows and screen edges. When 0, no space is reserved. |
| OffsetY     | int     | Pixels to leave between new and old windows and screen edges. When 0, no space is reserved. |

**Config File Elements under the UniqueNames-subsection of the Screen-section:**

| Keyword   | Type    | Description                                           |
|-----------|---------|-------------------------------------------------------|
| SetUnique | boolean | Decides if the feature is used or not. False or True. |
| Pre       | string  | String to place before the unique client number.      |
| Post      | string  | String to place after the unique client number.       |

**Config File Elements under the CmdDialog-section:**

| Keyword             | Type    | Description                                                                                                                 |
|---------------------|---------|-----------------------------------------------------------------------------------------------------------------------------|
| HistoryUnique       | boolean | If true, identical items in the history will only appear once where the most recently used is the first item. Default true. |
| HistorySize         | int     | Number of entries in the history that should be kept track of. Default 1024.                                                |
| HistoryFile         | string  | Path to history file where history is persisted between session. Default ~/.pekwm/history                                   |
| HistorySaveInterval | int     | Defines how often the history file should be saved counting each time the CmdDialog finish a command. Default 16.           |

**Config File Elements under the Menu-section:**

| Keyword        | Type    | Description                                                                                                                         |
|----------------|---------|-------------------------------------------------------------------------------------------------------------------------------------|
| DisplayIcons   | boolean | Defines whether menus should render their icons. Default true.                                                                       |
| FocusOpacity   | int     | Sets the opacity/transparency for focused Menus. A value of 100 means completely opaque, while 0 stands for completely transparent. |
| UnfocusOpacity | int     | Sets the opacity/transparency for unfocused Menus.                                                                                  |

**Icons = "MENU"**

| Keyword | Type            | Description                                                                                                                                                   |
|---------|-----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Minimum | width x height  | Defines minimum size of icons, if 0x0 no check is done. Default 16x16.                                                                                        |
| Maximum | width x height  | Defines maximum size of icons, if 0x0 no check is done. Default 16x16.                                                                                        |
| Select  | list of strings | Decides on what mouse events to select a menu entry. List is space separated and can include "ButtonPress, ButtonRelease, DoubleClick, Motion, MotionPressed" |
| Enter   | list of strings | Decides on what mouse events to enter a submenu. List is space separated and can include "ButtonPress, ButtonRelease, DoubleClick, Motion, MotionPressed"     |
| Exec    | list of strings | Decides on what mouse events to execute an entry. List is space separated anc can include "ButtonPress, ButtonRelease, DoubleClick, Motion, MotionPressed"    |

**Config File Elements under the Harbour-section:**

| Keyword      | Type    | Description                                                                                                                       |
|--------------|---------|-----------------------------------------------------------------------------------------------------------------------------------|
| Placement    | string  | Which edge to place the harbour on. One of Right, Left, Top, or Bottom.                                                           |
| Orientation  | string  | From what to which direction the harbour expands. One of TopToBottom, BottomToTop, RightToLeft, LeftToRight.                      |
| OnTop        | boolean | Whether or not the harbour is "always on top"                                                                                     |
| MaximizeOver | boolean | Controls whether maximized clients will cover the harbour (true), or if they will stop at the edge of the harbour (false).        |
| Head         | int     | When RandR or Xinerama is on, decides on what head the harbour resides on. Integer is the head number. Default 0.                 |
| HeadName     | string  | When RandR is on, decides on what head the harbour resides on by name (which could be found from `xrandr` command). A special name "primary" connects to the primary head. This option overwrites Head. |
| Opacity      | int     | Sets the opacity/transparency for the harbour. A value of 100 means completely opaque, while 0 stands for completely transparent. |

**Config File Elements under the DockApp-subsection of the Harbour-section:**

| Keyword | Type | Description                                                                                                                                                             |
|---------|------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| SideMin | int  | Controls the minimum size of dockapp clients. If a dockapp client is smaller than the minimum, it gets resized up to the SideMin value. Integer is a number of pixels.  |
| SideMax | int  | Controls the maximum size of dockapp clients. If a dockapp client is larger than the maximum, it gets resized down to the SideMax value. Integer is a number of pixels. |

### Screen Subsections

There are two subsections in the screen section - Placement and
UniqueNames. Placement can optionally have its own subsection. Sound
hard? It's really not! It's actually quite simple.

We'll start off with Placement. Placement has two options: Model, and
a 'Smart' subsection. Model is very simple, it's simply a list of
keywords that describes how to place new windows, such as "Smart
MouseCentered". Secondly, there's a Smart section, which describes how
pekwm computes where to place a new window in smart mode.

The second subsection, UniqueNames, lets you configure how pekwm
should handle similar client names. pekwm can add unique number
identifiers to clients that have the same title so that instead of
"terminal" and "terminal", you would end up with something like
"terminal" and "terminal \[2\]".

**Config File Elements under the Debug-section:**

| Keyword   | Type   | Description                                                            |
|-----------|--------|------------------------------------------------------------------------|
| File      | string | The location of the debug log file, such as ~/.pekwm/log               |
| Level     | string | The debug log level (err|warn|info|debug|trace)                        |

**Config File Elements under the Theme-section:**

| Keyword            | Type    | Description                                                  |
|--------------------|---------|--------------------------------------------------------------|
| BackgroundOverride | texture | Texture, that if set, overrides the theme background texture |

Configuring the menus
---------------------

The root menu is what you get when you (by default- See the [Mouse
Bindings](actions.md#actions) section) left-click on the root window
(also called the desktop). You can also configure the window menu,
which you get when you right-click on a window title.

### Basic Menu Syntax

As previously indicated, the root and window menus follow the rules
defined in [Common Syntax](#common-syntax). There aren't many possible
options, and they're all either within the main menu, or within a
submenu. This is all handled by a single file.

As an addition to the default menu types, RootMenu and WindowMenu, a user
can define any number of new menus following the same syntax.

Here's an example ~/.pekwm/menu file, where we have the usual RootMenu
and WindowMenu and our own most used applications menu called
MyOwnMenuName:

```
# Menu config for pekwm

# Variables
$TERM = "xterm -fn fixed +sb -bg black -fg white"

RootMenu = "pekwm" {
    Entry = "Term" { Actions = "Exec $TERM" }
    Entry = "Emacs" { Icon = "emacs.png"; Actions = "Exec $TERM -title emacs -e emacs -nw" }
    Entry = "Vim" { Actions = "Exec $TERM -title vim -e vi" }

    Separator {}

    Submenu = "Utils" {
        Entry = "XCalc" { Actions = "Exec xcalc" }
        Entry = "XMan" { Actions = "Exec xman" }
    }

    Separator {}

    Submenu = "pekwm" {
        Entry = "Reload" { Actions = "Reload" }
        Entry = "Restart" { Actions = "Restart" }
        Entry = "Exit" { Actions = "Exit" }
        Submenu = "Others" {
            Entry = "Xterm" { Actions = "RestartOther xterm" }
            Entry = "Twm" { Actions = "RestartOther twm" }
        }
        Submenu = "Themes" {
            Entry { Actions = "Dynamic ~/.pekwm/scripts/pekwm_themeset.pl" }
        }
    }
}

WindowMenu = "Window Menu" {
    Entry = "(Un)Stick" { Actions = "Toggle Sticky" }
    Entry = "(Un)Shade" { Actions = "Toggle Shaded" }

    ...

    SubMenu = "Send To" {
        Entry = "Workspace 1" { Actions = "SendToWorkspace 1" }
        Entry = "Workspace 2" { Actions = "SendToWorkspace 2" }
        Entry = "Workspace 3" { Actions = "SendToWorkspace 3" }
        Entry = "Workspace 4" { Actions = "SendToWorkspace 4; GoToWorkspace 4" }
    }
    
    ...

    Entry = "Close" { Actions = "Close" }
}

MyOwnMenuName = "Most used apps" {
	Entry = "Term" { Actions = "Exec $TERM" }
	Entry = "XCalc" { Actions = "Exec xcalc" }
	Entry = "Dillo" { Actions = "Exec dillo" }
}
```

### Menu Keywords

Here are the different elements that can be used in your root menu file.

**Root Menu Elements:**

Submenu (Name)

Begins a submenu. 'name' is what will appear in the root menu for the entry.

Entry (Name)

Begins a menu entry. 'Name' is the text shown in the menu for this entry.

Actions (Action)

Run an action. 'Action' is the action(s) to run. Most actions listed
in [Keys/mouse actions](actions.md#actions) will also work from the
root and window menus.

Icon (Image)

Set icon left of entry from image in icon path.

Separator

Adds a separator to the menu.

**Menu Actions:**

Exec

Exec makes pekwm execute the command that follows it.

Reload

When this is called, pekwm will re-read all configuration files without exiting.

Restart

This will cause pekwm to exit and re-start completely.

RestartOther

Quits pekwm and starts another application. The application to run is given as a parameter.

Exit

Exits pekwm. Under a normal X setup, This will end your X session.

Of course, in addition to these, many actions found from [Keys/mouse
actions](actions.md#actions) also work.

* * *

### Custom Menus

User can also define an unlimited amount of custom menus. They are
called with the ShowMenu action much like the Root and Window menus
are (see [Keys/mouse actions](actions.md#actions)).

In the example menu on this documentation, we created your own menu,
called 'MyOwnMenuName'. Basically, outside of the RootMenu and
WindowMenu sections, we open our own section called
'MyOwnMenuName'. This can of course be called whatever you want it to
be called, but do note that the menu names are case insensitive. This
means you can't have one menu called 'MyMostUsedApps' and one called
'mymostusedapps'.

Lets see that example again, simplified:

```
RootMenu = "pekwm" { ... }
WindowMenu = "Window Menu" { ... }
MyOwnMenuName = "Most used apps" {
	Entry = "Term" { Actions = "Exec $TERM" }
	Entry = "XCalc" { Actions = "Exec xcalc" }
	Entry = "Dillo" { Actions = "Exec dillo" }
}
```

We would call this new menu using the action 'ShowMenu MyOwnMenuName',
The menu would show 'Most used apps' as the menu title and list
'Term', 'XCalc' and 'Dillo' in the menu ready to be executed.

### Dynamic Menus

It is possible to use dynamic menus in pekwm, that is menus that
regenerate themselves whenever the menu is viewed. This is done with
the Dynamic keyword.

To use this feature, you need to put a dynamic entry in the
~/.pekwm/menu file with a parameter that tells pekwm what file to
execute to get the menu. This file can be of any language you prefer,
the main thing is that it outputs valid pekwm menu syntax inside a
Dynamic {} section. The syntax of dynamic entry looks like this:

```
Entry = "" { Actions = "Dynamic /path/to/filename" }
```

The input from a program that creates the dynamic content should
follow the general menu syntax encapsulated inside a Dynamic {}
section. Variables have to be included inside the dynamic menu for
them to work. A simple script to give pekwm dynamic menu content would
look like this:

```
#!/bin/bash
output=$RANDOM # gets a random number

echo "Dynamic {"
echo " Entry = \"$output\" { Actions = \"Exec xmessage $output\" }"
echo "}"

This script would output something like:

Dynamic {
 Entry = "31549" { Actions = "Exec xmessage 31549" }
}
```

Clients can access the PID and Window that was active when the script
was executed via the environment variables $CLIENT\_PID and
$CLIENT\_WINDOW. $CLIENT\_PID is only available if the client is being
run on the same host as pekwm.

Keyboard and Mouse Configuration
--------------------------------

pekwm allows you to remap almost all keyboard and mouse events.

### Mouse Bindings

The pekwm Mousebindings go in ~/.pekwm/mouse, and are very
simple. They're divided up into two groups: The 'where' and
'event'. Below is an example file:

```
FrameTitle {
	ButtonRelease = "1" { Actions = "Raise; Focus; ActivateClient" }
	ButtonRelease = "2" { Actions = "ActivateClient" }
	ButtonRelease = "Mod4 3" { Actions = "Close" }
	ButtonRelease = "3" { Actions = "ShowMenu Window" }
	ButtonRelease = "4" { Actions = "ActivateClientRel 1" }
	ButtonRelease = "5" { Actions = "ActivateClientRel -1" }
	DoubleClick = "2" { Actions = "Toggle Shaded" }
	DoubleClick = "1" { Actions = "MaxFill True True" }
	Motion = "1" { Threshold = "4"; Actions = "Move" }
	Motion = "Mod1 1" { Threshold = "4"; Actions = "Move" }
	Motion = "Mod4 1" { Threshold = "4"; Actions = "Move" }
	Motion = "2" { Threshold = "4"; Actions = "GroupingDrag True" }
	Motion = "Mod1 3" { Actions = "Resize" }
	Enter = "Any Any" { Actions = "Focus" }
}

OtherTitle {
	ButtonRelease = "1" { Actions = "Raise; Focus; ActivateClient" }
	ButtonRelease = "Mod4 3" { Actions = "Close" }
	DoubleClick = "2" { Actions = "Toggle Shaded" }
	DoubleClick = "1" { Actions = "MaxFill True True" }
	Motion = "1" { Threshold = "4"; Actions = "Move" }
	Motion = "Mod1 1" { Threshold = "4"; Actions = "Move" }
	Motion = "Mod4 1" { Threshold = "4"; Actions = "Move" }
	Motion = "Mod1 3" { Actions = "Resize" }
	Enter = "Any Any" { Actions = "Focus" }
}

Border {
	TopLeft     { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Resize TopLeft" } }
	Top         { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Move" } }
	TopRight    { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Resize TopRight" } }
	Left        { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Resize Left" } }
	Right       { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Resize Right" } }
	BottomLeft  { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Resize BottomLeft" } }
	Bottom      { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Resize Bottom" } }
	BottomRight { Enter = "Any Any" { Actions = "Focus" }; ButtonPress = "1" { Actions = "Resize BottomRight" } }
}

ScreenEdge {
	Down {
		ButtonRelease = "3" { Actions = "ShowMenu Root" }
		ButtonRelease = "2" { Actions = "ShowMenu Goto" }
	}
	Up {
		ButtonRelease = "3" { Actions = "ShowMenu Root" }
		ButtonRelease = "2" { Actions = "ShowMenu Goto" }
		ButtonRelease = "Mod1 4" { Actions = "GoToWorkspace Right" }
		ButtonRelease = "Mod1 5" { Actions = "GoToWorkspace Left" }
	}
	Left {
		Enter = "Mod1 Any" { Actions = "GoToWorkspace Left" }
		ButtonRelease = "3" { Actions = "ShowMenu Root" }
		ButtonRelease = "1" { Actions = "GoToWorkspace Left" }
		DoubleClick = "1" { Actions = "GoToWorkspace Left" }
		ButtonRelease = "2" { Actions = "ShowMenu Goto" }
		ButtonRelease = "4" { Actions = "GoToWorkspace Right" }
		ButtonRelease = "5" { Actions = "GoToWorkspace Left" }
	}
	Right {
		Enter = "Mod1 Any" { Actions = "GoToWorkspace Right" }
		ButtonRelease = "3" { Actions = "ShowMenu Root" }
		ButtonRelease = "1" { Actions = "GoToWorkspace Right" }
		DoubleClick = "1" { Actions = "GoToWorkspace Right" }
		ButtonRelease = "2" { Actions = "ShowMenu Goto" }
		ButtonRelease = "4" { Actions = "GoToWorkspace Right" }
		ButtonRelease = "5" { Actions = "GoToWorkspace Left" }
	}
}

Client {
	ButtonPress = "1" { Actions = "Focus" }
	ButtonRelease = "Mod1 1" { Actions = "Focus; Raise" }
	ButtonRelease = "Mod4 1" { Actions = "Lower" }
	Motion = "Mod1 1" { Actions = "Focus; Raise; Move" }
	Motion = "Mod4 1" { Actions = "Focus; Raise; Move" }
	Motion = "Mod1 2" { Threshold = "4"; Actions = "GroupingDrag True" }
	Motion = "Mod1 3" { Actions = "Resize" }
	Enter = "Any Any" { Actions = "Focus" }
}

Root {
	ButtonRelease = "3" { Actions = "ShowMenu Root" }
	ButtonRelease = "2" { Actions = "ShowMenu Goto" }
	ButtonRelease = "4" { Actions = "GoToWorkspace Right" }
	ButtonRelease = "5" { Actions = "GoToWorkspace Left" }
	ButtonRelease = "1" { Actions = "HideAllMenus" }
}

Menu {
	Enter = "Any Any" { Actions = "Focus" }
	ButtonRelease = "2" { Actions = "Toggle Sticky" }
	Motion = "1" { Threshold = "10"; Actions = "Move" }
	ButtonRelease = "3" { Actions = "Close" }
}

Other {
	Enter = "Any Any" { Actions = "Focus" }
	ButtonRelease = "Mod4 3" { Actions = "Close" }
	Motion = "1" { Actions = "Focus; Raise; Move" }
	Motion = "Mod1 1" { Actions = "Focus; Raise; Move" }
}
```

Below you will find, the different fields defined. The actions themselves can be
found in the [Keys/mouse actions](actions.md#actions) section.

**'Where' fields:**

FrameTitle

On a regular window's Titlebar.

OtherTitle

On menu/cmdDialog/etc pekwm's own window's Titlebar.

Border

On the window's borders. See [Border Subsection](#mouse-only-actions) for more information.

ScreenEdge

On the screen edges. See [ScreenEdge Subsection](#mouse-only-actions) for more information.

Client

Anywhere on the window's interior. It's best to use a keyboard modifier with these.

Root

On the Root window (also called the 'desktop').

Menu

On the various menus excluding their titlebars.

Other

On everything else that doesn't have its own section.

**'Event' fields:**

ButtonPress

A single click

ButtonRelease

A single click that activates once the button is released

DoubleClick

A double click

Motion

Clicking, holding, and Dragging.

Enter

Defines how to act when mouse pointer enters a place defined by the 'where' field.

Leave

Defines how to act when mouse pointer leaves a place defined by the 'where' field.

EnterMoving

Defines how to act when a dragged window enters a ScreenEdge. Only works with screen edges.

Definitions work like this:

```
'Where' {
	'Event' = "optional modifiers, like mod1, ctrl, etc and a mouse button" {
		Actions = "actions and their parameters"
	}
	'Event' = "optional modifiers, like mod1, ctrl, etc and a mouse button" {
		Actions = "actions and their parameters"
	}
}
```

> **Additional notes.**
> 
> Modifiers and mouse buttons can be defined as "Any" which is useful
> for Enter and Leave events. Any also applies as none. Motion events
> have a threshold argument; this is the number of pixels you must
> drag your mouse before they begin to work. Multiple actions can be
> defined for a single user action. Example:

```
Motion = "1" { Actions = "Move"; Treshold = "3" }
ButtonPress = "1" { Actions = "Raise; ActivateClient" }
```

### Border Subsection

The Border subsection in ~/.pekwm/mouse defines the actions to take
when handling the window borders.

```
Border {
	TopLeft {
		Enter = "Any Any" { Actions = "Focus" }
		ButtonPress = "1" { Actions = "Resize TopLeft" }
	}
}
```

It's subsections refer to the frame part in question. They are: Top,
Bottom, Left, Right, TopLeft, TopRight, BottomLeft, and
BottomRight. In these subsections you can define events and actions as
usual.

### ScreenEdge Subsection

The ScreenEdge subsection in ~/.pekwm/mouse defines the actions to
take when an event happens on the specified screenedge.

```
ScreenEdge {
    Left {
        Enter = "Mod1 Any" { Actions = "GoToWorkspace Left" }
        ButtonPress = "3" { Actions = "ShowMenu Root" }
        ButtonPress = "1" { Actions = "GoToWorkspace Left" }
        ButtonPress = "2" { Actions = "ShowMenu Goto" }
        ButtonPress = "4" { Actions = "GoToWorkspace Right" }
        ButtonPress = "5" { Actions = "GoToWorkspace Left" }
    }
}
```

It has four subsections: Up, Down, Left, and Right, that all refer to
the screen edge in question. In these subsections you can give events
and actions as usual.

### Key Bindings

The pekwm keybindings go in ~/.pekwm/keys, and are even more simple
than the mouse bindings. Here's the format:

```
KeyPress = "optional modifiers like mod1, ctrl, etc and the key" {
	Actions = "action and the parameters for the action, if they are needed"
}
```

Multiple actions can be drawn from one keypress. The actions are
separated from each other with a semicolon:

```
Keypress = "Ctrl t" { Actions = "Exec xterm; Set Maximized True True; Close" }
```

Here's a small fragment of an example keys file; you can see a full
version in ~/.pekwm/keys. As with the mouse, you can see the full list
of actions in the [Keys/mouse actions](actions.md#actions)
section.

```
Global {
	# Moving in frames
	KeyPress = "Mod1 Tab" { Actions = "NextFrame EndRaise" }
	KeyPress = "Mod1 Shift Tab" { Actions = "PrevFrame EndRaise" }
	KeyPress = "Mod1 Ctrl Tab" { Actions = "NextFrameMRU EndRaise" }
	KeyPress = "Mod1 Ctrl Shift Tab" { Actions = "PrevFrameMRU EndRaise" }
	# Simple window management
	KeyPress = "Mod4 M" { Actions = "Toggle Maximized True True" }
	KeyPress = "Mod4 G" { Actions = "Maxfill True True" }
	KeyPress = "Mod4 F" { Actions = "Toggle FullScreen" }
	KeyPress = "Mod4 Return" { Actions = "MoveResize" }
	# Wm actions
	Chain = "Ctrl Mod4 P" {
		KeyPress = "Delete" { Actions = "Reload" }
		KeyPress = "Next" { Actions = "Restart" }
		KeyPress = "End" { Actions = "Exit" }
		KeyPress = "Prior" { Actions = "RestartOther twm" }
	}
}

MoveResize {
	KeyPress = "Left" { Actions = "MoveHorizontal -10" }
	KeyPress = "Right" { Actions = "MoveHorizontal 10" }
	KeyPress = "Up" { Actions = "MoveVertical -10" }
	KeyPress = "Down" { Actions = "MoveVertical 10" }
	Keypress = "Mod4 Left" { Actions = "ResizeHorizontal -10" }
	Keypress = "Mod4 Right" { Actions = "ResizeHorizontal 10" }
	Keypress = "Mod4 Up" { Actions = "ResizeVertical -10" }
	Keypress = "Mod4 Down" { Actions = "ResizeVertical 10" }
	Keypress = "s" { Actions = "MoveSnap" }
	Keypress = "Escape" { Actions = "Cancel" }
	Keypress = "Return" { Actions = "End" }
}

Menu {
	KeyPress = "Down" { Actions = "NextItem" }
	KeyPress = "Up" { Actions = "PrevItem" }
	KeyPress = "Left" { Actions = "LeaveSubmenu" }
	KeyPress = "Right" { Actions = "EnterSubmenu" }
	KeyPress = "Return" { Actions = "Select" }
	KeyPress = "Escape" { Actions = "Close" }
}

InputDialog {
	KeyPress = "BackSpace" { Actions = "Erase" }
	KeyPress = "Right" { Actions = "CursNext" }
	KeyPress = "Left" { Actions = "CursPrev" }
	KeyPress = "Up" { Actions = "HistPrev" }
	KeyPress = "Down" { Actions = "HistNext" }
	KeyPress = "Delete" { Actions = "Clear" }
	KeyPress = "Return" { Actions = "Exec" }
	KeyPress = "Escape" { Actions = "Close" }
	KeyPress = "Any Any" { Actions = "Insert" }
}
```

As you might have noticed, the file consist of four sections. These
sections are Global, MoveResize, Menu and InputDialog. The first
section, Global, contains all the generic actions.

The MoveResize section has the keybindings that will get used when the
MoveResize action is called.

Menu section contains the keys that are used when the ShowMenu action
is called. E.g. these are the keys you use to browse thru the menus of
pekwm. Note that while ShowMenu is active, the Global keybindings are
also listened. If a keybinding is same in both Menu and Global
sections, keybindings in Menu section override the global keybinding
as long as a menu is active.

Finally, the InputDialog section allow for tuning of what keys are
available for line editing when the CmdDialog window that enables the
user to enter pekwm actions for running windows is active.

Keys can be identified with their XString name or with their
keycode. Both can be found out using the X application **xev**. If you
want to use a keycode, prefix it with **#**.

### Keychains

pekwm also supports keychains. Keychain syntax follows the general
config syntax and looks like this:

```
Chain = "modifiers and key" {
	Chain = "modifiers and key" {
		KeyPress = "modifiers and key" { Actions = "actions and their parameters" }
	}
	Keypress = "modifiers and key" { Actions = "actions and their parameters" }
}
```

It might seem complicated at start but once you look into it, it is
fairly nice and logical. This syntax supports as many nested Chains as
you might want.

Now for some examples. Here we have a simple nested chain that lets
you press Ctrl+Alt+M, then M, then M, V or H to toggle maximized
attribute into Full/Vertical or Horizontal, and a simpler one level
chain that brings up the root menu.

```
Chain = "Ctrl Mod4 A" {
	Chain = "M" {
		KeyPress = "M" { Actions = "Toggle Maximized True True" }
		KeyPress = "V" { Actions = "Toggle Maximized False True" }
		KeyPress = "H" { Actions = "Toggle Maximized True False" }
	}
}
Chain = "Ctrl Mod4 M" {
	KeyPress = "R" { Actions = "ShowMenu Root" }
}
```

This next rule is a pure show-off, it lets you type in 'test' and then
executes xterm. Note that this will make you unable to type the
character 't' to any programs.

```
Chain = "t" { Chain = "e" { Chain = "s" {
	Keypress = "t" { Actions = "Exec xterm" }
} } }
```

[Actions](actions.md)

The pekwm start file
--------------------

The ~/.pekwm/start file is the simplest of all of pekwm's config
files. It's a simple shell script that's run on pekwm
startup. Therefore, to run, it needs to be set executable with **chmod
+x ~/.pekwm/start**.

Why would anyone use start rather than just use their ~/.xinitrc file?
Well, the answer is, the start file is executed during the pekwm
initialization phase - therefore, it gets re-executed when you issue a
pekwm 'restart'.

Here's an example pekwm start file. Be careful to place long running
applications to the background, or you will seem stuck when trying to
start pekwm.

```
#!/bin/sh

xmessage 'hi. pekwm started.' &
some_command &
```

***

[< Previous (Basic Usage)](basic-usage.md)
\- [Up](README.md)
\- [(Autoproperties) Next >](autoproperties.md)
