III. Configuration
==================

The configuration part of this documentation is focused on providing complete documentation for all config files located in your ~/.pekwm directory.

**Table of Contents**

7\. [The Pekwm Common Syntax for Config Files](#config-syntax)

8\. [The main config file](#config-configfile)

9\. [Configuring the menus](#config-menu)

10\. [Autoproperties](#config-autoprops)

11\. [Keyboard and Mouse Configuration](#config-keys_mouse)

12\. [The pekwm start file](#config-start)

13\. [Pekwm themes](#config-theme)

* * *

Chapter 7. The Pekwm Common Syntax for Config Files
===================================================

* * *

#### 7.1. Basic Syntax

All pekwm config files (with the exception of the start file- see [start file](#config-start) ) follow a common syntax for options.

\# comment
// another comment
/\*
	yet another comment
\*/

$VARIABLE = "Value"
$\_ENVIRONMENT\_VARIABLE = "Value"
INCLUDE = "another\_configuration.cfg"
COMMAND = "program to execute and add the valid config syntax it outputs here"

# Normal format
Section = "Name" {
	Event = "Param" {
		Actions = "action parameter; action parameter; $VAR $\_VARIABLE"
	}
}

// Compressed format
Section = "Name" { Event = "Param" { Actions = "action parameters; action parameters; $VAR $\_VARIABLE" } }

You can usually modify the spacing and line breaks, but this is the "Correct" format, so the documentation will try to stick to it.

Events can be combined into the same line by issuing a semicolon between them. Actions can be combined into the same user action by issuing a semicolon between them. You can use an INCLUDE anywhere in the file.

Pekwm has a vars file to set common variables between config files. Variables are defined in vars and the file is INCLUDEd from the configuration files.

Comments are allowed in all config files, by starting a comment line with # or //, or enclosing the comments inside /\* and \*/.

* * *

#### 7.2. Template Syntax

The pekwm configuration parser supports the use of templates to reduce typing. Template support is currently available in autoproperties and theme configuration files. Template configuration is not fully compatible with the non-template syntax and thus requires activation to not break backwards compatibility. To enable template parsing start the configuration file with the following:

Require {
	Templates = "True"
}

Defining templates is just like creating an ordinary section, but uses the special name Define. Below is an example defining a template named NAME:

Define = "NAME" {
  Section = "Sub" {
    ValueSub = "Sub Value"
  }
  Value = "Value"
}

The above defined template can later be used by using the magic character @. The below example shows usage of the template NAME in a two sections named Name and NameOverride overriding one of the template values:

Section = "Name" {
  @NAME
}
Section = "NameOverride" {
  @NAME
  Value = "Overridden value"
}

The above example is equivalent of writing the following:

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

* * *

#### 7.3. Variables In Pekwm Config Files

Pekwm config enables you to use both internal to pekwm variables, as well as global system variables. Internal variables are prefixed with a **$**, global variables with **$\_**.

\# examples of how to set both type of variables
$INTERNAL = "this is an internal variable"
$\_GLOBAL = "this is a global variable"

# examples of how to read both type of variables
RootMenu = "Menu" {
	Entry = "$\_GLOBAL" { Actions = "xmessage $INTERNAL" }
}

There is one special global variable pekwm handles. It is called $\_PEKWM\_CONFIG\_FILE. This global variable is read when pekwm starts, and it's contents will be used as the default config file. It will also be updated to point to the currently active config file if needed.

Variables can probably be defined almost anywhere, but it's probably a better idea to place them at the top of the file, outside of any sections.

* * *

Chapter 8. The main config file
===============================

The main config file is where all the base config stuff goes.

* * *

#### 8.1. Basic Config

As previously indicated, the config file follows the rules defined in [Common Syntax](#config-syntax).

Here's an example ~/.pekwm/config file:

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

	Placement {
		Model = "Smart"
		WorkspacePlacements = "Smart;MouseCentered"
		TransientOnParent = "True"
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

* * *

#### 8.2. Config File Keywords

Here's a table showing the different elements that can be used in your config file. Remember that 'boolean' means 'true' or 'false' and that all values should be placed inside quotes.

**Config File Elements under the Files-section:**

Keys (string)

The location of the keys file, such as ~/.pekwm/keys

Menu (string)

The location of the menu file, such as ~/.pekwm/menu

Start (string)

The location of the start file, such as ~/.pekwm/start

AutoProps (string)

The location of the autoprops file, such as ~/.pekwm/autoproperties

Theme (string)

The location of the Theme directory, such as ~/.pekwm/themes/themename

Icons (string)

The location of the Icons directory, such as ~/.pekwm/icons

**Config File Elements under the MoveResize-section:**

EdgeAttract (int)

The distance from screen edge required for the window to snap against it in pixels.

EdgeResist (int)

The distance from screen edge required for the window moving to start being resisted in pixels.

WindowAttract (int)

The distance from other clients that a window will snap against them to in pixels.

WindowResist (int)

The distance from other clients that a window movement will start being resisted.

OpaqueMove (boolean)

If true, turns on opaque Moving

OpaqueResize (boolean)

If true, turns on opaque Resizing

**Config File Elements under the Screen-section:**

Workspaces (int)

Number of workspaces enabled.

WorkspacesPerRow (int)

Number of workspaces on each row. Value < 1 fits all workspaces on a single row.

WorkspaceNames (string)

List of names for workspaces separated by ;.

ShowFrameList (boolean)

Controls whether a list of all available frames on the workspace is displayed during the NextFrame/PrevFrame actions.

ShowStatusWindow (boolean)

Controls whether a size/position info window is shown when moving or resizing windows.

ShowStatusWindowCenteredOnRoot (boolean)

Controls whether a size/position info window is shown centered on the current head or the current window.

ShowClientID (boolean)

Should Client IDs be displayed in window titles.

ShowWorkspaceIndicator (int)

Show WorkspaceIndicator for N milliseconds. If set to < 1, the WorkspaceIndicator is disabled.

WorkspaceIndicatorScale (int)

Changes the size of the WorkspaceIndicator, higher value means smaller size.

WorkspaceIndicatorOpacity (int)

Sets the opacity/transparency of the WorkspaceIndicator. A value of 100 means completely opaque, while 0 stands for completely transparent.

Note that a Composite Manager needs to be running for this feature to take effect.

FocusNew (boolean)

Toggles if new windows should be focused when they pop up.

FocusNewChild (boolean)

Toggles if new transient windows should be focused when they pop up if the window they are transient for is focused.

PlaceNew (boolean)

Toggles if new windows should be placed using the rules found in the Placement subsection, or just opened on the top left corner of your screen.

ReportAllClients (boolean)

Toggles if all clients in a frame or only the active one should be reported and thus displayed in pager applications etc.

TrimTitle (string)

This string contains what pekwm uses to trim down overlong window titles. If it's empty, no trimming down is performed at all.

FullscreenAbove (boolean)

Toggles restacking of windows when going to and from fullscreen mode. Windows are restacked to the top of all windows when going to fullscreen and to the top of their layer when being restored from fullscreen.

FullscreenDetect (boolean)

Toggles detection of broken fullscreen requests setting clients to fullscreen mode when requesting to be the size of the screen. Default true.

HonourRandr (boolean)

Toggles reading of XRANDR information, this can be disabled if the display driver gives both Xinerama and Randr information and only of the two is correct. Default true.

HonourAspectRatio (boolean)

Toggles if pekwm respects the aspect ratio of clients (XSizeHints). Default true.

EdgeSize (int) (int) (int) (int)

How many pixels from the edge of the screen should screen edges be. Parameters correspond to the following edges: top bottom left right. A value of 0 disables edges.

EdgeIndent (boolean)

Toggles if the screen edge should be reserved space.

DoubleClicktime (int)

Time, in milliseconds, between clicks to be counted as a doubleclick.

**Config File Elements under the Placement-subsection of the Screen-section:**

TransientOnParent (bool)

Set to true if you want the transient windows to be mappend on their "parent" window (tiling layouters might ignore this option).

Model (string)

*   Smart - Tries to place windows where no other window is present
    
*   MouseCentered - Places the center of the window underneath the current mouse pointer position
    
*   MouseTopLeft - Places the top-left corner of the window under the pointer
    
*   MouseNotUnder - Places windows on screen corners avoiding the current mouse cursor position.
    
*   CenteredOnParent - Places transient windows at center of their parent window.
    

WorkspacePlacements (string)

List of placement models for the workspaces separated by ;. For an explanation of the allowed options see "Model" above.

**Config File Elements under the Smart-subsection of the Placement-subsection:**

Row (boolean)

Whether to use row or column placement, if true, uses row.

TopToBottom (boolean)

If false, the window is placed starting from the bottom.

LeftToRight (boolean)

If false, the window is placed starting from the right.

OffsetX (int)

Pixels to leave between new and old windows and screen edges. When 0, no space is reserved.

OffsetY (int)

Pixels to leave between new and old windows and screen edges. When 0, no space is reserved.

**Config File Elements under the UniqueNames-subsection of the Screen-section:**

SetUnique (boolean)

Decides if the feature is used or not. False or True.

Pre (string)

String to place before the unique client number.

Post (string)

String to place after the unique client number.

**Config File Elements under the CmdDialog-section:**

HistoryUnique (boolean)

If true, identical items in the history will only appear once where the most recently used is the first item. Default true.

HistorySize (integer)

Number of entries in the history that should be kept track of. Default 1024.

HistoryFile (string)

Path to history file where history is persisted between session. Default ~/.pekwm/history

HistorySaveInterval (int)

Defines how often the history file should be saved counting each time the CmdDialog finish a command. Default 16.

**Config File Elements under the Menu-section:**

DisplayIcons (boolean)

Defines wheter menus should render their icons. Default true.

FocusOpacity (int)

Sets the opacity/transparency for focused Menus. A value of 100 means completely opaque, while 0 stands for completely transparent.

UnfocusOpacity (int)

Sets the opacity/transparency for unfocused Menus.

Icons = "MENU"

Minimum (width x height)

Defines minimum size of icons, if 0x0 no check is done. Default 16x16.

Maximum (width x height)

Defines maximum size of icons, if 0x0 no check is done. Default 16x16.

Select (list of strings)

Decides on what mouse events to select a menu entry. List is space separated and can include "ButtonPress, ButtonRelease, DoubleClick, Motion, MotionPressed"

Enter (list of strings)

Decides on what mouse events to enter a submenu. List is space separated and can include "ButtonPress, ButtonRelease, DoubleClick, Motion, MotionPressed"

Exec (list of strings)

Decides on what mouse events to execute an entry. List is space separated anc can include "ButtonPress, ButtonRelease, DoubleClick, Motion, MotionPressed"

**Config File Elements under the Harbour-section:**

Placement (string)

Which edge to place the harbour on. One of Right, Left, Top, or Bottom.

Orientation (string)

From what to which direction the harbour expands. One of TopToBottom, BottomToTop, RightToLeft, LeftToRight.

OnTop (boolean)

Whether or not the harbour is "always on top"

MaximizeOver (boolean)

Controls whether maximized clients will cover the harbour (true), or if they will stop at the edge of the harbour (false).

Head (int)

When RandR or Xinerama is on, decides on what head the harbour resides on. Integer is the head number.

Opacity (int)

Sets the opacity/transparency for the harbour. A value of 100 means completely opaque, while 0 stands for completely transparent.

**Config File Elements under the DockApp-subsection of the Harbour-section:**

SideMin (int)

Controls the minimum size of dockapp clients. If a dockapp client is smaller than the minimum, it gets resized up to the SideMin value. Integer is a number of pixels.

SideMax (int)

Controls the maximum size of dockapp clients. If a dockapp client is larger than the maximum, it gets resized down to the SideMax value. Integer is a number of pixels.

* * *

#### 8.3. Screen Subsections

There are two subsections in the screen section - Placement and UniqueNames. Placement can optionally have its own subsection. Sounds hard? It's not! It's really quite simple.

We'll start off with Placement. Placement has two options: Model, and a 'Smart' subsection. Model is very simple, it's simply a list of keywords that describes how to place new windows, such as "Smart MouseCentered". Secondly, there's a Smart section, which describes how pekwm computes where to place a new window in smart mode.

The second subsection, UniqueNames, lets you configure how pekwm should handle similar client names. Pekwm can add unique number identifiers to clients that have the same title so that instead of "terminal" and "terminal", you would end up with something like "terminal" and "terminal \[2\]".

* * *

Chapter 9. Configuring the menus
================================

The root menu is what you get when you (by default- See the [Mouse Bindings](#config-keys_mouse-mouse) section) left-click on the root window (also called the desktop). You can also configure the window menu, which you get when you right-click on a window title.

* * *

#### 9.1. Basic Menu Syntax

As previously indicated, the root and window menus follow the rules defined in [Common Syntax](#config-syntax). There aren't many possible options, and they're all either within the main menu, or within a submenu. This is all handled by a single file.

As addition to the default menu types, RootMenu and WindowMenu, user can define any number of new menus following the same syntax.

Here's an example ~/.pekwm/menu file, where we have the usual RootMenu and WindowMenu and our own most used applications menu called MyOwnMenuName:

\# Menu config for pekwm

# Variables
$TERM = "xterm -fn fixed +sb -bg black -fg white"

RootMenu = "Pekwm" {
    Entry = "Term" { Actions = "Exec $TERM &" }
    Entry = "Emacs" { Icon = "emacs.png"; Actions = "Exec $TERM -title emacs -e emacs -nw &" }
    Entry = "Vim" { Actions = "Exec $TERM -title vim -e vi &" }

    Separator {}

    Submenu = "Utils" {
        Entry = "XCalc" { Actions = "Exec xcalc &" }
        Entry = "XMan" { Actions = "Exec xman &" }
    }

    Separator {}

    Submenu = "Pekwm" {
        Entry = "Reload" { Actions = "Reload" }
        Entry = "Restart" { Actions = "Restart" }
        Entry = "Exit" { Actions = "Exit" }
        Submenu = "Others" {
            Entry = "Xterm" { Actions = "RestartOther xterm" }
            Entry = "Twm" { Actions = "RestartOther twm" }
        }
        Submenu = "Themes" {
            Entry { Actions = "Dynamic ~/.pekwm/scripts/pekwm\_themeset.pl" }
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
	Entry = "Term" { Actions = "Exec $TERM &" }
	Entry = "XCalc" { Actions = "Exec xcalc &" }
	Entry = "Dillo" { Actions = "Exec dillo &" }
}

* * *

#### 9.2. Menu Keywords

Here are the different elements that can be used in your root menu file.

**Root Menu Elements:**

Submenu (Name)

Begins a submenu. 'name' is what will appear in the root menu for the entry.

Entry (Name)

Begins a menu entry. 'Name' is the text shown in the menu for this entry.

Actions (Action)

Run an action. 'Action' is the action(s) to run. Most actions listed in [Keys/mouse actions](#config-keys_mouse-actions) will also work from the root and window menus.

Icon (Image)

Set icon left of entry from image in icon path.

Separator

Adds a separator to the menu.

**Menu Actions:**

Exec

Exec makes pekwm to execute the command that follows it. Make sure the program gets backgrounded. Put an '&' at the end of the action if it doesn't do this on it's own.

Reload

When this is called, pekwm will re-read all configuration files without exiting.

Restart

This will cause pekwm to exit and re-start completely.

RestartOther

Quits pekwm and starts another application. The application to run is given as a parameter.

Exit

Exits pekwm. Under a normal X setup, This will end your X session.

Of course, in addition to these, many actions found from [Keys/mouse actions](#config-keys_mouse-actions) also work.

* * *

#### 9.3. Custom Menus

User can also define an unlimited amount of custom menus. They are called with the ShowMenu action much like the Root and Window menus are (see [Keys/mouse actions](#config-keys_mouse-actions)).

In the example menu on this documentation, we created your own menu, called 'MyOwnMenuName'. Basically, outside of the RootMenu and WindowMenu sections, we open our own section called 'MyOwnMenuName'. This can of course be called whatever you want it to be called, but do note that the menu names are case insensitive. This means you can't have one menu called 'MyMostUsedApps' and one called 'mymostusedapps'.

Lets see that example again, simplified:

RootMenu = "Pekwm" { ... }
WindowMenu = "Window Menu" { ... }
MyOwnMenuName = "Most used apps" {
	Entry = "Term" { Actions = "Exec $TERM &" }
	Entry = "XCalc" { Actions = "Exec xcalc &" }
	Entry = "Dillo" { Actions = "Exec dillo &" }
}

We would call this new menu using the action 'ShowMenu MyOwnMenuName', The menu would show 'Most used apps' as the menu title and list 'Term', 'XCalc' and 'Dillo' in the menu ready to be executed.

* * *

#### 9.4. Dynamic Menus

It is possible to use dynamic menus in pekwm, that is menus that regenerate themselves whenever the menu is viewed. This is done with the Dynamic keyword.

To use this feature, you need to put a dynamic entry in the ~/.pekwm/menu file with a parameter that tells pekwm what file to execute to get the menu. This file can be of any language you prefer, the main thing is that it outputs valid pekwm menu syntax inside a Dynamic {} section. The syntax of dynamic entry looks like this:

Entry = "" { Actions = "Dynamic /path/to/filename" }

The input from a program that creates the dynamic content should follow the general menu syntax encapsulated inside a Dynamic {} section. Variables have to be included inside the dynamic menu for them to work. A simple script to give pekwm dynamic menu content would look like this:

#!/bin/bash
output=$RANDOM # gets a random number

echo "Dynamic {"
echo " Entry = \\"$output\\" { Actions = \\"Exec xmessage $output\\" }"
echo "}"

This script would output something like:

Dynamic {
 Entry = "31549" { Actions = "Exec xmessage 31549" }
}

Clients can access the PID and Window that was active when the script was executed via the environment variables $CLIENT\_PID and $CLIENT\_WINDOW. $CLIENT\_PID is only available if the client is being run on the same host as pekwm.

* * *

Chapter 10. Autoproperties
==========================

* * *

#### 10.1. What are Autoproperties?

"Autoproperties" is short for "Automatic Properties". This is pekwm's way of setting certain things up for applications based on the window's internal id. You can set up a lot of things, such as size, iconified state, start location, grouped state (automatically having one window group to another), workspace to start on, whether it has a border or titlebar, and more. It is also possible to automatically modify window titles and to decide the order of applications on the harbour with autoproperties.

* * *

#### 10.2. Basic Autoproperties Syntax

The ~/.pekwm/autoproperties file follows the rules in [Common Syntax](#config-syntax). This file can become rather complicated, but it's also the most powerful of any of pekwm's config files.

The one important thing to remember is the Property tag. This identifier tells us where to apply properties. It means which windows to apply it on. To find out the two terms, use **xprop WM\_CLASS** and click on your window. Below you'll find a bash/zsh function which will give you the correct string for this file. You can also specify a regexp wildcard, such as ".\*,opera", which means anything for the first word, opera for the second.

propstring () {
  echo -n 'Property '
  xprop WM\_CLASS | sed 's/.\*"\\(.\*\\)", "\\(.\*\\)".\*/= "\\1,\\2" {/g'
  echo '}'
}

Autoproperties have an both an old and new style matching clients. The new style was introduced to support using configuration template overwriting.

In addition with WM\_CLASS, pekwm also can identify clients by their title string (**xprop WM\_NAME** or **xprop \_NET\_WM\_NAME**).

\# New syntax, requires Require { Templates = "True" }
Property = "^dillo,^Dillo,,Dillo: pekwm.org - not just another windowmanager" {
	ApplyOn = "Start New"
	Layer = "OnTop"
}

# Old syntax
Property = "^dillo,^Dillo" {
        Title = "Dillo: pekwm.org - not just another windowmanager"
	ApplyOn = "Start New"
	Layer = "OnTop"
}

Or by their role (**xprop WM\_WINDOW\_ROLE**):

\# New syntax, requires Require { Templates = "True" }
Property = "^gaim,^Gaim,preferences" {
	ApplyOn = "New"
	Skip = "Menus"
}

# Old syntax
Property = "^gaim,^Gaim" {
	Role = "preferences"
	ApplyOn = "New"
	Skip = "Menus"
}

Pekwm can rewrite window titles. This is done in a separate TitleRules section, where one defines properties on which clients to use the rewriting and then a regexp rule of what to do to that clients title. These rules do not affect the actual WM\_NAME string. You can use Role and Title keywords to narrow down the clients the titlerule applies to. A simple rule that would change "Title: this is the title" to "this is the title" looks like this:

TitleRules {
	Property = "^foo,^bar" {
		Rule = "/Title: (.\*)/\\\\1/"
	}
}

In pekwm, you can make certain windows have their own decoration set. The different decorations are defined in the theme, and they are connected to client windows with an autoproperty. These autoproperties reside in their own DecorRules subsection and look like this:

DecorRules {
	Property = "^foo,^bar" {
		Decor = "TERM"
	}
}

It's also possible to decide the order of applications that start in the harbour. As with TitleRules and DecorRules, there is it's own separate section for this purpose called Harbour. Position is a signed int and order goes: "1 2 3 0 0 0 -3 -2 -1", and so on. That looked cryptic. Worry not. Basically, a Position number of 0 means the application will be placed in the middle. If the number is positive, the application will be placed before the zero-positioned applications. If the number is negative, the applications will be placed after the zero-position ones. So the positive numbered show up first in your harbour, then the zero numbered, and after the zeros come the negatively numbered applications. I hope that is clear, the next part is tricky. The larger the value of the base number the closer to the zero applications they will be. So the smaller the base number the closer to the ends of the harbour the application will be. Position 1 would be the first application to show up on the harbour. And similarly Position -1 would be the last application on the harbour. If you have application on the harbour that do not match any of the property rules on the Harbour section, they will act as if you had given them Position 0. Applications with the same Position will show up next to each other in the order they are launched. In our example below, obpager will always be placed the last on the harbour.

Harbour {
	Property = "^obpager,^obpager" {
		Position = "-1";
	}
}

If you want certain autoproperties to be only applied when you are on a specific workspace, you can add a workspace section. The following example sets an autoproperty that removes the border and titlebar from xterm on the second and third workspace. Please keep in mind that we start counting with 0.

Workspace = "1 2" {
    Property = "xterm,XTerm" {
        ApplyOn = "Start New Reload"
        Border = "False"
        Titlebar = "False"
    }
}

Here's an example ~/.pekwm/autoproperties file:

Property = ".\*,^xmms" {
	ApplyOn = "Start New"
	Layer = "0"
	Sticky = "True"
}

Property = "^xclock,^XClock" {
	ApplyOn = "Start New"
	FrameGeometry = "100x100+0-0"
	Border = "False"; Titlebar = "False"
	Sticky = "True"
	Layer = "Desktop"
}

Property = "^dillo,^Dillo" {
	ApplyOn = "Start New"
	Group = "browsers" {
		Size = "30"
		Behind = "True"
		Global = "False"
	}
}

TitleRules {
	Property = "^dillo,^Dillo" {
		Rule = "/Dillo: (.\*)/\\\\1 \[dillo\]/"
	}
	Property = "^opera,^opera" {
		Rule = "/...:... - (.\*) - Opera .\*/\\\\1 \[opera\]/"
	}
}

DecorRules {
	Property = "^.term,^XTerm" {
		Decor = "TERM"
	}
}

Harbour {
	Property = "^obpager,^obpager" {
		Position = "-1"
	}
}

> **Regular Expressions!**
> 
> The pekwm autoproperties file uses Regular Expression syntax for wildcards. Regular expressions can be really confusing to people with no experience with them. A good rule of thumb is: "Anywhere you'd think to use '\*', use '.\*'". Also, '^' matches the beginning of a string, '$' matches the end, and '.' is any single character. Pekwm has some special flags to that modifies regular expression matching. Specifying regular expressions in the form /pattern/flags allow flags to be set. The supported flags are ! for inverting the match and i for case insensitive matches. Explaining the syntax of regular expressions goes beyond the scope of this documentation. You might want to look at [http://www.regularexpressions.info/](http://www.regularexpressions.info/) or [http://en.wikipedia.org/wiki/Regular\_expressions](http://en.wikipedia.org/wiki/Regular_expressions).

* * *

#### 10.3. Advanced Autoproperties

Below is a list of the different actions available to you in your autoproperties file; These are the actual Auto Properties. They can take four types of arguments: bool, integer, string, or geom. A bool is either True (1) or False (0). An Integer is a number, negative or positive. A string is any string, it's used as an identifier. Finally, geom is an X Geometry String by the form: "\[=\]\[<width>{xX}<height>\]\[{+-}<xoffset>{+-}<yoffset>\]" (see: man 3 XParseGeometry). Examples are 200x300+0+0, 0x500+200+300, 20x10+0+50, et cetera.

**Exhaustive Autoprops List**

AllowedActions (string) , DisallowedActions (string)

A list of actions to allow/deny performing on a client.

*   "Move" ((Dis)allow moving of the client window)
    
*   "Resize" ((Dis)allow resizing of the client window)
    
*   "Iconify" ((Dis)allow iconifying of the client window)
    
*   "Shade" ((Dis)allow shading of the client window)
    
*   "Stick" ((Dis)allow setting sticky state on the client window)
    
*   "MaximizeHorizontal" ((Dis)allow maximizing the client window horizontally)
    
*   "MaximizeVertical" ((Dis)allow maximizing the client window vertically)
    
*   "Fullscreen" ((Dis)allow setting the client window in fullscreen mode)
    
*   "SetWorkspace" ((Dis)allow changing of workspace)
    
*   "Close" ((Dis)allow closing)
    

ApplyOn (string)

A list of conditions of when to apply this autoprop (so be sure to include this in your property), consisting of

*   "New" (Applies when the application first starts)
    
*   "Reload" (Apply when pekwm's config files are reloaded)
    
*   "Start" (Apply if window already exists before pekwm starts/restarts. Note when using grouping Start will not take workspaces in account)
    
*   "Transient" (Apply to Transient windows as well as normal windows. Dialog boxes are commonly transient windows)
    
*   "TransientOnly" (Apply to Transient windows only. Dialog boxes are commonly transient windows)
    
*   "Workspace" (Apply when the window is sent to another workspace)
    

Border (bool)

Window starts with a border

CfgDeny (string)

A list of conditions of when to deny things requested by the client program, consisting of

*   "Above" (Ignore client request to always place window above other windows)
    
*   "ActiveWindow" (Ignore client requests for showing and giving input focus)
    
*   "Below" (Ignore client request to always place window below other windows)
    
*   "Fullscreen" (Ignore client request to set window to fullscreen mode)
    
*   "Hidden" (Ignore client request to show/hide window)
    
*   "MaximizedHorz" (Ignore client request to maximize window horizontally)
    
*   "MaximizedVert" (Ignore client request to maximize window vertically)
    
*   "Position" (Ignore client requested changes to window position)
    
*   "Size" (Ignore client requested changes to window size)
    
*   "Stacking" (Ignore client requested changes to window stacking)
    
*   "Strut" (Ignore client request for reserving space in the screen corners, typically done by panels and the like)
    
*   "Tiling" (Tiling layouters should leave this window floating)
    

ClientGeometry (geom)

X Geometry String showing the initial size and position of the client, excluding the possible pekwm titlebar and window borders.

Decor (string)

Use the specified decor for this window. The decor has to be defined in the used theme. The decor is chosen by the first match in order: AutoProperty, TypeRules, DecorRules.

Focusable (bool)

Toggles if this client can be focused while it's running.

FocusNew (bool)

Toggles if this client gets focused when it initially pops up a window.

FrameGeometry (geom)

X Geometry String showing the initial size and position of the window frame. Window frame includes the client window and the possible pekwm titlebar and window borders. If both ClientGeometry and FrameGeometry are present, FrameGeometry overrides the ClientGeometry.

Fullscreen (bool)

Window starts in fullscreen mode

Group (string)

Defines the name of the group. Also the section that contains all the grouping options. They are:

*   Behind (bool) - If true makes new clients of a group not to become the active one in the group.
    
*   FocusedFirst (bool) - If true and there are more than one frame where the window could be autogrouped into, the currently focused frame is considered the first option.
    
*   Global (bool) - If true makes new clients start in a group even if the group is on another workspace or iconified.
    
*   Raise (bool) - If true makes new clients raise the frame they open in.
    
*   Size (integer) - How many clients should be grouped in one group.
    

Iconified (bool)

Window starts Iconified

Layer (string)

Windows layer. Makes the window stay under or above other windows. Default layer is "Normal". Possible parameters are (listed from the bottommost to the uppermost):

*   Desktop
    
*   Below
    
*   Normal
    
*   OnTop
    
*   Harbour
    
*   AboveHarbour
    
*   Menu
    

MaximizedHorizontal (bool)

Window starts Maximized Horizontally

MaximizedVertical (bool)

Window starts Maximized Vertically

Opacity (int int)

Sets the focused and unfocused opacity values for the window. A value of 100 means completely opaque, while 0 stands for completely transparent.

Note that a Composite Manager needs to be running for this feature to take effect.

PlaceNew (bool)

Toggles the use of placing rules for this client.

Role (string)

Apply this autoproperty on clients that have a WM\_WINDOW\_ROLE hint that matches this string. String is a regexp like: "^Main".

Shaded (bool)

Window starts Shaded

Skip (string)

A list of situations when to ignore the defined application and let the user action skip over it, consisting of

*   "Snap" (Do not snap to this window while moving windows)
    
*   "Menus" (Do not show this window in pekwm menus other than the icon menu)
    
*   "FocusToggle" (Do not focus to this window when doing Next/PrevFrame)
    

Sticky (bool)

Window starts Sticky (present on all workspaces)

Title (string)

Apply this autoproperty on clients that have a title that matches this string. String is a regexp like: "^Saving".

Titlebar (bool)

Window starts with a TitleBar

Workspace (integer)

Which workspace to start program on.

* * *

#### 10.4. AutoGrouping

The last thing to know is autogrouping. Autogrouping is actually very simple, although it might be a bit confusing at first. Group is an identifier, it's just a string, (in my example, we'll call it netwin). Size tells how many clients to group together in one frame.

The example: We want to autogroup Sylpheed and Opera together, allowing as many instances of the program windows to be grouped as there are. Here's the Autoprops section for that:

Property = ".\*,^opera" {
	Group = "netwin" {
		Size = "0"
	}
	ApplyOn = "New Start Reload"
}
Property = ".\*,^Sylpheed" {
	Group = "netwin" {
		Size = "0"
	}
	ApplyOn = "New Start Reload Transient"
}

This creates two rules: "For any window matching '.\*,^opera', group these in the 'netwin' group. Apply this on pekwm start/reload and when new windows matching this property are opened, but do not include dialog windows", and "For any window matching '.\*,^Sylpheed', group in the 'netwin' group. Apply on pekwm start/reload and when new windows matching this property are opened, also include possible dialog windows to the group. Open the window to the group but do not bring it upmost automatically".

To group unlimited windows together, use size 0.

Also note that you can have as many Group identifiers as you want. Autogrouping is a very flexible system. Try playing around with it.

* * *

#### 10.5. TypeRules, autoproperties controlling \_NET\_WM\_WINDOW\_TYPE

The TypeRules decides how the \_NET\_WM\_WINDOW\_TYPE should be interpreted. The \_NET\_WM\_WINDOW\_TYPE hint gives the application writer possibility to inform the window manager what kind of window it is creating.

TypeRules are defined in the TypeRules section of the ~/.pekwm/autoproperties file. A sample section could look something like this:

TypeRules {
    ...

    Property = "MENU"  {
        Titlebar = "False"
        Border = "False"
        Skip = "FocusToggle Menus Snap"
    }

    ...
}

Using TypeRules are done the same way as with [Advanced Autoproperrties](#config-autoprops-adv) but the property is matched based on the value of \_NET\_WM\_WINDOW\_TYPE. Supported values are available in the list below.

**Supported values**

Desktop

A desktop window such as the window containing desktop icons on the Gnome desktop.

Dock

Toolbar

Menu

Utility

Splash

Application startup screen usually presenting loading progress.

Dialog

Dialogs prompting for information such as "Save as" dialogs.

Normal

Any other window, can be used to set default autoproperties.

* * *

#### 10.6. Getting more help

Autoprops can be a daunting topic. If you've read everything here in the docs and are still having problems, feel free to hit the [IRC](#devel-irc) channel and ask. Check the [Common questions and answers](#faq-answers) before asking. Remember that: "IF YOU WANT AN ANSWER TO YOUR QUESTION, YOU HAD BETTER HAVE ALREADY READ THE DOCUMENTATION".

* * *

Chapter 11. Keyboard and Mouse Configuration
============================================

Pekwm allows you to remap almost all keyboard and mouse events.

* * *

#### 11.1. Mouse Bindings

The pekwm Mousebindings go in ~/.pekwm/mouse, and are very simple. They're divided up into two groups: The 'where' and 'event'. Below is an example file:

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

Below are defined the different fields. The actions themselves can be found in the [Keys/mouse actions](#config-keys_mouse-actions) section.

**'Where' fields:**

FrameTitle

On a regular window's Titlebar.

OtherTitle

On menu/cmdDialog/etc pekwm's own window's Titlebar.

Border

On the window's borders. See [Border Subsection](#config-keys_mouse-border) for more information.

ScreenEdge

On the screen edges. See [ScreenEdge Subsection](#config-keys_mouse-screenedge) for more information.

Client

Anywhere on the window's interior. It's best to use a keyboard modifier with these.

Root

On the Root window (also called the 'desktop').

Menu

On the various menus excluding their titlebars.

Other

On everything else that doesn't have it's own section.

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

'Where' {
	'Event' = "optional modifiers, like mod1, ctrl, etc and a mouse button" {
		Actions = "actions and their parameters"
	}
	'Event' = "optional modifiers, like mod1, ctrl, etc and a mouse button" {
		Actions = "actions and their parameters"
	}
}

> **Additional notes.**
> 
> Modifiers and mouse buttons can be defined as "Any" which is useful for Enter and Leave events. Any also applies as none. Motion events have a threshold argument. This is the number of pixels you must drag your mouse before they begin to work. Multiple actions can be defined for a single user action. Example:

Motion = "1" { Actions = "Move"; Treshold = "3" }
ButtonPress = "1" { Actions = "Raise; ActivateClient" }

* * *

#### 11.2. Border Subsection

The Border subsection in ~/.pekwm/mouse defines the actions to take when handling the window borders.

Border {
	TopLeft {
		Enter = "Any Any" { Actions = "Focus" }
		ButtonPress = "1" { Actions = "Resize TopLeft" }
	}
}

It's subsections refer to the frame part in question. They are: Top, Bottom, Left, Right, TopLeft, TopRight, BottomLeft, and BottomRight. In these subsections you can define events and actions as usual.

* * *

#### 11.3. ScreenEdge Subsection

The ScreenEdge subsection in ~/.pekwm/mouse defines the actions to take when an event happens on the specified screenedge.

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

It has four subsections: Up, Down, Left, and Right, that all refer to the screen edge in question. In these subsections you can give events and actions as usual.

* * *

#### 11.4. Key Bindings

The pekwm keybindings go in ~/.pekwm/keys, and are even more simple than the mouse bindings. Here's the format:

KeyPress = "optional modifiers like mod1, ctrl, etc and the key" {
	Actions = "action and the parameters for the action, if they are needed"
}

Multiple actions can be given for one keypress. The actions are separated from each other with a semicolon:

Keypress = "Ctrl t" { Actions = "Exec xterm; Set Maximized True True; Close" }

Here's a small fragment of an example keys file; you can see a full version in ~/.pekwm/keys. As with the mouse, you can see the full list of actions in the [Keys/mouse actions](#config-keys_mouse-actions) section.

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
	Chain = "Ctrl Mod1 P" {
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

As you might have noticed, the file consist of four sections. These sections are Global, MoveResize, Menu and InputDialog. The first section, Global, contains all the generic actions.

The MoveResize section has the keybindings that will get used when the MoveResize action is called.

Menu section contains the keys that are used when the ShowMenu action is called. E.g. these are the keys you use to browse thru the menus of pekwm. Note that while ShowMenu is active, the Global keybindings are also listened. If a keybinding is same in both Menu and Global sections, keybindings in Menu section override the global keybinding as long as a menu is active.

Finally, the InputDialog section allow for tuning of what keys are available for line editing when the CmdDialog window that enables the user to enter pekwm actions for running windows is active.

Keys can be identified with their XString name or with their keycode. Both can be found out using the X application **xev**. If you want to use a keycode, prefix it with **#**.

* * *

#### 11.5. Keychains

Pekwm also supports keychains. Keychain syntax follows the general config syntax and looks like this:

Chain = "modifiers and key" {
	Chain = "modifiers and key" {
		KeyPress = "modifiers and key" { Actions = "actions and their parameters" }
	}
	Keypress = "modifiers and key" { Actions = "actions and their parameters" }
}

It might seem complicated at start but once you look into it, it is fairly nice and logical. This syntax supports as many nested Chains as you might want.

Now for some examples. Here we have a simple nested chain that lets you press Ctrl+Alt+M, then M, then M, V or H to toggle maximized attribute into Full/Vertical or Horizontal, and a simpler one level chain that brings up the root menu.

Chain = "Ctrl Mod1 A" {
	Chain = "M" {
		KeyPress = "M" { Actions = "Toggle Maximized True True" }
		KeyPress = "V" { Actions = "Toggle Maximized False True" }
		KeyPress = "H" { Actions = "Toggle Maximized True False" }
	}
}
Chain = "Ctrl Mod1 M" {
	KeyPress = "R" { Actions = "ShowMenu Root" }
}

This next rule is a pure show-off, it lets you type in 'test' and then executes xterm. Note that this will make you unable to type the character 't' to any programs.

Chain = "t" { Chain = "e" { Chain = "s" {
	Keypress = "t" { Actions = "Exec xterm" }
} } }

* * *

#### 11.6. Keys/Mouse actions and window attributes

Here is the list of all possible actions and attributes. First table shows all toggleable attributes. Toggleable attributes are controlled using the _Set_, _Unset_ and _Toggle_ actions. Examples below.

 Keypress = "Mod4 s"       { Actions = "Toggle Shaded" }
 Keypress = "Mod4 m"       { Actions = "Toggle Maximized True True" }
 Keypress = "Mod4 t"       { Actions = "Set Tagged" }
 Keypress = "Mod4 Shift t" { Actions = "Unset Tagged" }

**Toggleable attributes:**

Maximized (bool bool)

If a frame is maximized. Two parameters, first one decides if the frame is maximized horizontally, the second if it is maximized vertically.

Fullscreen

If a frame should be fullscreen. Fullscreen frame takes over the whole desktop ignoring any harbour or struts and becomes decorless.

Shaded

If a frame should be shaded (so that only the titlebar shows until it's unset or toggled off).

Sticky

If a frame should be sticky so it appears on every workspace.

AlwaysOnTop

If frame should always be on top of other frames.

AlwaysBelow

If a frame should always be below other frames.

Decor

When used with Set/Toggle it takes an additional parameter for the decorname that is set/toggled. Unset just removes the last "override"-decor.

DecorBorder

If frame should have borders.

DecorTitlebar

If frame should have a titlebar.

Iconified

If a frame should be iconified.

Opaque

If the frame should be fully opaque (ie: disable the opacity setting).

Tagged (bool)

If a frame should swallow all new clients until unset or toggled off. One parameter, if true new clients open in the background. Defaults to false.

Marked

If a frame is marked for later attaching (with AttachMarked).

Skip (string)

If a frame should be ignored on specified places, string is one of

*   menus
    
*   focustoggle
    
*   snap
    

CfgDeny (string)

When things to be done to this window requested by the client program should be denied, string is one of:

*   above (don't let client place window above other windows)
    
*   activewindow (don't let client give input focus)
    
*   below (don't let client place window below other windows)
    
*   fullscreen (don't let client set window fullscreen mode)
    
*   hidden (don't let client hide window)
    
*   maximizedhorz (don't let client maximize window horizontally)
    
*   maximizedvert (don't let client maximize a window vertically)
    
*   position (don't let the client move the window)
    
*   size (don't let the client resize the window)
    
*   stacking (don't allow the client to raise or lower the window)
    
*   tiling (Tiling layouters should leave this window floating)
    

Title (string)

Changes the clients titlebar text to string when set. Unsetting returns the clients title text back to the client specified one.

HarbourHidden

If set, harbour and anything in it will be hidden from the screen.

GlobalGrouping

If all autogrouping should be in use or not. By default it's set, as in autogrouping is enabled.

**Keys/Mouse Actions:**

Focus

Gives focus to a frame.

UnFocus

Removes focus from a frame.

Set (one of toggleable attributes)

Makes toggleable attributes set.

UnSet (one of toggleable attributes)

Unsets toggleable attributes.

Toggle (one of toggleable attributes)

Toggles toggleable attributes.

MaxFill (bool bool)

Acts much like Maximize, but considers other frames while doing it. Instead of filling the whole screen, it only fills to the borders of neighboring frames. Takes two parameters, first one decides if the frame is maxfilled horizontally, the second if it should be maxfilled vertically.

GrowDirection (string)

Grows the frame in one of the directions up to the edge of the head. String is one of up, down, left, right.

Close

Closes a client window.

CloseFrame

Closes a frame and all client windows in it.

Kill

Kills a client window, use if close doesn't work.

SetGeometry (string int)

Sets the geometry of a frame. The first option is a geometry string that XParseGeometry can parse. The second one specifies the head number that geometry should be relative to. It defaults to -1 which means the geometry is relative to the whole screen.

Raise (bool)

Raises a frame above other frames. If bool is true raises a frame and all of the currently active clients child/parent windows above other frames.

Lower (bool)

Lowers a frame under other frames. If bool is true lowers a frame and all of the currently active clients child/parent windows under other frames.

ActivateOrRaise

If the frame this action is used on is not focused, focuses it. If the frame is focused, raises it. If used on a groups titlebar, activates the selected client of the group.

ActivateClientRel (int)

Moves the focus and raises a client inside a frame relative to the currently selected client. Int is 1 to move right, -1 to move left.

MoveClientRel (int)

Moves the current clients position inside the current frame. Int is 1 to move right, -1 to move left.

ActivateClient

Activates a client of a frame.

Mouse-specific

ActivateClientNum (int)

Activates the #th client of a frame. Int is the client number counting from left.

Keygrabber-specific

Resize (string)

Resizes a frame. String is one of top, bottom, left, right, topleft, topright, bottomleft, bottomright.

Mouse-specific (parameters frameborder-specific)

Move

Moves a frame.

Mouse-specific

MoveResize

Activates the keyboard move and resize.

Keygrabber-specific

GroupingDrag (bool)

Drags windows in and out of frames, if parameter is true dragged windows go in the background of a frame.

Mouse-specific

WarpToWorkspace (string)

Makes a dragged window warp to specified workspace when a it's moved over a screen edge. String is one:

*   next - send to the next workspace, if on last workspace, wrap to the first one.
    
*   prev - send to the previous workspace, if on first workspace, wrap to the last one.
    
*   left - send to the previous workspace
    
*   right - send to the next workspace
    
*   int - integer is a workspace number to send to to
    

ScreenEdge specific mouse binding

MoveToHead (int)

Moves the frame to the same relative position on another head. The window is shrinked to fit if it is larger than the new head.

MoveToEdge (string)

Moves the frame to the specified place on the screen. String is one of TopLeft, TopEdge, TopRight, RightEdge, BottomRight, BottomEdge, BottomLeft, LeftEdge, Center, TopCenterEdge, BottomCenterEdge, LeftCenterEdge, RightCenterEdge.

Keygrabber-specific

NextFrame (string boolean)

Focuses the next frame. String is one of:

*   alwaysraise - raise windows while toggling them
    
*   endraise - raise the selected client
    
*   neverraise - do not raise the selected client (unless it's iconified)
    
*   tempraise - raise the selected client but keep the order of the other windows
    

If boolean is true, also goes thru iconified windows. Defaults to false.

PrevFrame (string boolean)

Focuses the previous frame. String is:

*   alwaysraise - raise windows while toggling them
    
*   endraise - raise the selected client
    
*   neverraise - do not raise the selected client (unless it's iconified)
    
*   tempraise - raise the selected client but keep the order of the other windows
    

If boolean is true, also goes thru iconified windows. Defaults to false.

NextFrameMRU (string boolean)

Focuses the next frame so that the last focused windows will get switched to first. String is:

*   alwaysraise - raise windows while toggling them
    
*   endraise - raise the selected client
    
*   neverraise - do not raise the selected client (unless it's iconified)
    
*   tempraise - raise the selected client but keep the order of the other windows
    

If boolean is true, also goes thru iconified windows. Defaults to false.

PrevFrameMRU (string boolean)

Focuses the previous frame so that the last focused windows will get switched to first. String is:

*   alwaysraise - raise windows while toggling them
    
*   endraise - raise the selected client
    
*   neverraise - do not raise the selected client (unless it's iconified)
    
*   tempraise - raise the selected client but keep the order of the other windows
    

If boolean is true, also goes thru iconified windows. Defaults to false.

FocusDirectional (string bool)

Focuses the first window on the direction specified, and optionally raises it. Takes two options, first one is the direction and the second specifies if the focused frame should be raised or not. Bool defaults to True. String is one of up, down, left, right

AttachMarked

Attachs all marked clients to the current frame.

AttachClientInNextFrame

Attachs client to the next frame.

AttachClientInPrevFrame

Attachs client to the previous frame.

FindClient (string)

Searches the client list for a client that has a title matching the given regex string.

GotoClientID (string)

Shows and focuses a client based on the Client ID given as a parameter.

Detach

Detach the current client from its frame.

SendToWorkspace (string)

Sends a frame to the specified workspace. String is one of:

*   next - send to the next workspace, if on last workspace, wrap to the first one.
    
*   prev - send to the previous workspace, if on first workspace, wrap to the last one.
    
*   left - send to the previous workspace
    
*   right - send to the next workspace
    
*   prevv - send to the previous (vertical) workspace, if on last workspace, wrap to the first one.
    
*   up - send to the previous (vertical) workspace.
    
*   nextv - sed to the next (vertical) workspace, if on last workspace, wrap to the first one.
    
*   down -
    
*   last - send to workspace you last used before the current
    
*   int - integer is a workspace number to send to to
    

GotoWorkspace (string)

Changes workspaces. String is one of:

*   left - go to the previous workspace on current row.
    
*   prev - go to the previous workspace on current row, if on first workspace, wrap to the last one on current row.
    
*   right - go to the next workspace on current row.
    
*   next - go to the next workspace on current row, if on last workspace, wrap to the first one on current row.
    
*   leftn - go to the previous workspace.
    
*   prevn - go to the previous workspace, if on first workspace, wrap to the last.
    
*   rightn - go to the next workspace.
    
*   nextn - go to the next workspace, if on last workspace, wrap to the first.
    
*   prevv -
    
*   up -
    
*   nextv -
    
*   down -
    
*   last - go to workspace you last used before the current
    
*   int - integer is a workspace number to go to
    

Exec (string)

Executes a program, string is a path to an executable file.

Reload

Reloads pekwm configs.

Keygrabber-specific

Restart

Restarts pekwm.

Keygrabber-specific

RestartOther

Quits pekwm and starts the program you specify. String is a path to an executable file.

Keygrabber-specific

Exit

Exits pekwm.

ShowCmdDialog (string)

Shows the command dialog that can be used to input pekwm actions. If it's a window specific action, it affects the window focused when CmdDialog was summoned. If entered action doesn't match any valid pekwm action, pekwm tries to Exec it as a shell command. Takes an optional string as a parameter. This string will then be pre-filled as the initial value of the dialog.

ShowSearchDialog (string)

Shows the search dialog that can be used to search for clients and when selected the client will be activated. Takes an optional string as a parameter. This string will then be pre-filled as the initial value of the dialog.

ShowMenu (string bool)

Shows a menu. String is menu type from below list or user defined menu name (see [Custom Menus](#config-menu-custommenus)):

*   root - shows your application menu
    
*   icon - shows iconified windows
    
*   goto - shows currently active clients
    
*   gotoclient - shows all open clients
    
*   window - shows a window specific menu
    
*   decor - shows possible decorations in the current theme
    
*   attachclient - allows to attach clients in current frame
    
*   attachframe - allows to attach whole frame in current frame
    
*   attachclientinframe - allows attaching current client in any other frame
    
*   attachframeinframe - allows attaching current frame in any other frame
    

Bool is true for sticky menus, false for click to vanish. Defaults to false.

HideAllMenus

Closes all pekwm menus.

SendKey

Send a key, possibly with modifiers, to the active window.

SetOpacity (int int)

Sets the Focused and Unfocused opacity values for the active window. 100 stands for fully opaque while 0 is completely transparent.

**MoveResize actions:**

MoveHorizontal (int)

Moves a frame horizontally. Int is amount of pixels and can be negative.

Moveresize-specific keybinding

MoveVertical (int)

Moves a frame vertically. Int is amount of pixels and can be negative.

Moveresize-specific keybinding

ResizeHorizontal (int)

Resizes a frame horizontally. Int is amount of pixels and can be negative.

Moveresize-specific keybinding

ResizeVertical (int)

Resizes a frame vertically. Int is amount of pixels and can be negative.

Moveresize-specific keybinding

MoveSnap

Snaps the frame to the closest frames or screenedges.

Moveresize-specific keybinding

Cancel

Cancels all moveresize actions and keeps the frame how it was before them.

Moveresize-specific keybinding

End

Acknowledges the moveresize actions and moves/resizes the frame as wished.

Moveresize-specific keybinding

**Menu actions:**

NextItem

Goes to next menu item.

Menu-specific keybinding

PrevItem

Goes to previous menu item.

Menu-specific keybinding

GotoItem (int)

Goes to item number int. First item in menu is 1.

Menu-specific keybinding

Select

Selects the current menu item.

Menu-specific keybinding

EnterSubmenu

Enters a submenu.

Menu-specific keybinding

LeaveSubmenu

Leaves a submenu.

Menu-specific keybinding

**InputDialog actions:**

Insert

Allows for the keypress to be inputted to the text field of InputDialog. Usually used to allow any other keys than the ones used for InputDialog.

InputDialog-specific keybinding

Erase

Erases the previous character according to the cursor position.

InputDialog-specific keybinding

Clear

Clears the whole InputDialog line.

InputDialog-specific keybinding

ClearFromCursor

Erases all characters after the current cursor position.

InputDialog-specific keybinding

Exec

Finishes input and executes the the data

Close

Closes an InputDialog.

InputDialog-specific keybinding

CursNext

Moves InputDialog cursor one characer space to right.

InputDialog-specific keybinding

CursPrev

Moves InputDialog cursor one characer space to left.

InputDialog-specific keybinding

CursEnd

Moves InputDialog cursor to the end of the line.

InputDialog-specific keybinding

CursBegin

Moves InputDialog cursor to the beginning of the line.

InputDialog-specific keybinding

HistNext

Get next history item previously used in InputDialog.

InputDialog-specific keybinding

HistPrev

Get previous history item previously used in InputDialog.

InputDialog-specific keybinding

* * *

Chapter 12. The pekwm start file
================================

The ~/.pekwm/start file is the simplest of all of pekwm's config files. It's a simple shell script that's run on pekwm startup. Therefore, to run, it needs to be set executable with **chmod +x ~/.pekwm/start**.

Why anyone would use start rather than just use their ~/.xinitrc file? Well, the answer is, the start file is executed during the pekwm initialization phase - therefore, it gets re-executed when you issue a pekwm 'restart'.

Here's an example pekwm start file. Be careful to place long running applications to the background, or you will seem stuck when trying to start pekwm.

#!/bin/sh

xmessage 'hi. pekwm started.' &
some\_command &

* * *

Chapter 13. Pekwm themes
========================

This section aims to documenting the pekwm theme structure. It's rather cryptic at first look, sorry.

Please use existing themes as real life examples and base when it comes to making your own.

* * *

13.1. Guidelines
----------------

It is strongly recommended and expected that theme tarballs are labeled for the pekwm version they are made and tested with. The filename format should be theme\_name-pekwm\_version.\[tar.gz|tgz|tar.bz2|tbz\]. For example silly\_theme-pekwm\_0.1.5.tar.bz2.

It is also highly recommended that theme directories are named in a similar fashion. However, for stable releases this is not mandatory, the tarball filename is enough. If you're building for a GIT revision, mention it in as many places as possible.

The silly theme from above would contain a directory structure as follows:

silly\_theme-pekwm\_0.1.5/
pekwm\_0.1.5/theme
pekwm\_0.1.5/menubg.png
pekwm\_0.1.5/submenu.png

The theme file header should contain at least the themes name, the pekwm version the theme is for, address to reach the theme maker/porter or get an updated theme, and a last modified date. Changelog entries won't hurt if you aren't the original theme author. For example:

\# silly, a PekWM 0.1.5 theme by shared (themes@adresh.com)
# This theme is available from hewphoria.com.
# Last modified 20060529.

# Extract this theme directory under ~/.pekwm/themes/ and the
# themes menu will pick it up automatically.

# Changelog:
# 2006-05-29 HAX0ROFUNIVERSE <hawt@haxorland.invalid>
#  \* REWROTE EVERYTHING WITH CAPS LOCK ON,
#    CAPS LOCK IS CRUISE CONTROL FOR COOL!

Try to stick to the theme syntax and rather than deleting entries please use the EMPTY texture.

* * *

13.2. Attribute names used, explanations, possible values, examples
-------------------------------------------------------------------

Here is the explanation of Attributes names of themes

**Attributes:**

pixels

An integer, amount of pixels.

example: "2"

size

Pixels vertically times pixels horizontally.

example: "2x2"

percent

Any percent value from 1 to 100.

example: "87"

toggle

sets a value as true (1) or false (0).

example: "true"

padding

Free pixels from top, free pixels under, free pixels from left, free pixels from right.

example: "2 2 2 2"

decorname

Name for decoration, any name can be used and applied to windows with autoproperties or the set decor action. The list below includes names with special meaning:

*   _DEFAULT_
    
    Defines decorations to all windows unless overridden with another decoration set (REQUIRED).
    
*   _ATTENTION_
    
    Defines the decoration for windows that set the urgency/demand-attention hint.
    
*   _BORDERLESS_
    
    Defines decorations for borderless windows (recommended).
    
*   _INPUTDIALOG_
    
    Defines decorations for input dialogs, such as the CmdDialog.
    
*   _MENU_
    
    Defines decorations for menus.
    
*   _STATUSWINDOW_
    
    Defines decorations for the command dialog.
    
*   _TITLEBARLESS_
    
    Defines decorations for titlebarless windows (recommended, should be there if your theme looks nasty when toggled titlebarless).
    
*   _WORKSPACEINDICATOR_
    
    Defines decorations for the workspace indicator.
    

colour

A colour value in RGB format.

example: "#FFFFFF"

imagename

Name of the imagefile with an option after the #

*   _#fixed_
    
    Image is fixed size.
    
*   _#scaled_
    
    Image will be scaled to fit the area it's defined for.
    
*   _#tiled_
    
    Image will be repeated as many times as needed to fill the area it's defined for. This is the default if no option is specified.
    

texture

Any valid texture. Valid textures are:

*   _EMPTY_
    
    No texture (transparent).
    
*   _SOLID colour size_
    
    A solid colour texture of defined colour and size.
    
*   _SOLIDRAISED colour colour colour pixels pixels toggle toggle toggle toggle size_
    
    A solid colour texture with a 3D look of defined colours, form and size. First colour defines the main fill colour, second the highlight colour used on the left and top parts of the texture, third the highlight colour on the bottom and right parts of the texture. First pixel amount defines how fart apart the 3D effects are from each other, second pixel amount is how thick the bordering will be (both pixels default to 1). The four toggles are used to tell which raised corners are to be drawn. This is useful for example when defining solidraised frame corner pieces. The order is Top, Bottom, Left, Right (not unlike that used in padding). As example: "True False True False" (or 1 0 1 0) could mean you want to draw the TopLeft piece of a solidraised window border. Size should explain itself, see above.
    
*   _IMAGE imagename_
    
    An image texture using the defined imagename
    

fontstring

Defines a font. Chopped to parts by # marks. First the font type (XFT or X11), then the font name, then the text orientation, then shadow offsets. Some fields can be omitted.

example: "XFT#Verdana:size=10#Left#1 1" example: "-misc-fixed-\*-\*-\*-\*-14-\*-\*-\*-\*-\*-\*-1#Center#1 1"

buttonactions

Buttonactions work alike what you are used from the mouse config, first mouse button number pressed when this action should happen, then any standard pekwm actions.

example: "1" { Actions = "Close" }

* * *

13.3. Theme structure
---------------------

### 13.3.1. PDecor

The block for decoration sets, any amount of Decor sections can exist inside this block.

* * *

#### 13.3.1.1. Decor

A list of blocks with theme specifications the various types of decorations.

* * *

##### 13.3.1.1.1. Title

Theming of the frame.

*   Height (pixels): Amount of pixels the titlebar should height.
    
*   HeightAdapt (boolean): If true, Height is adapted to fit the Title font.
    
*   Pad (pixels t,l,r,b): How many pixels are left around a title text.
    
*   Focused (texture): Background texture for a focused titlebar.
    
*   UnFocused (texture): Background texture for an unfocused titlebar.
    
*   WidthMin (pixels): Minimum width of title in pixels, will also place the titlebar outside of the window borders. Use 0 to place titlebar inside borders.
    
*   WidthMax (percent): Maximum width of titles relative to window width, when this value ends up being smaller than the value in WidthMin, WidthMin is overridden.
    
*   WidthSymetric (boolean): Set true to constant width titles or false to use titles that only are as big as the clients title text string requires (note, asymmetric width is not fully implemented yet, always set this true for now to avoid problems).
    

* * *

###### 13.3.1.1.1.1. Tab

Theming of a titlebar tabs.

*   Focused (texture): Background texture for a tab of a focused window.
    
*   Unfocused (texture): Background texture for a tab of an unfocused window.
    
*   FocusedSelected (texture): Background texture for the currently selected tab of a focused window.
    
*   UnFocusedSelected (texture): Background texture for the currently selected tab of an unfocused window.
    

* * *

###### 13.3.1.1.1.2. FontColor

Theming of font colors.

*   Focused (colour colour): Text colour for a tab of a focused window. second value is the shadow colour.
    
*   Unfocused (colour colour): Text colour for a tab of an unfocused window. second value for shadow.
    
*   FocusedSelected (colour colour): Text colour for the currently selected tab of a focused window. second value for shadow.
    
*   UnFocusedSelected (colour colour): Text colour for the currently selected tab of an unfocused window. second value for shadow.
    

* * *

###### 13.3.1.1.1.3. Font

Theming of the titlebar fonts.

*   Focused (fontstring): Font of the text of a tab of a focused window.
    
*   Unfocused (fontstring): Font of the text of a tab of an unfocused window.
    
*   FocusedSelected (fontstring): Font of the text of the currently selected tab of a focused window.
    
*   UnFocusedSelected (fontstring): Font of the text of the currently selected tab of an unfocused window.
    

* * *

###### 13.3.1.1.1.4. Separator

Theming of the tab separator.

*   Focused (texture): Separator texture for a focused window.
    
*   Unfocused (texture): Separator texture for an unfocused window.
    

* * *

###### 13.3.1.1.1.5. Buttons

Theming of titlebar buttons.

* * *

13.3.1.1.1.5.1. Right = "Name"

Places the button on the right end of the titlebar.

* * *

13.3.1.1.1.5.2. Left = "Name"

Places the button on the left end of the titlebar.

*   Focused (texture): Texture for button of a focused window.
    
*   Unfocused (texture): Texture for button of an unfocused window.
    
*   Pressed (texture): Texture for button that is pressed.
    
*   Hover (texture): Texture for button when pointer is placed on it.
    
*   SetShape (bool): If true, the shape of the button is derived from the alpha-channel. If false, the alpha-channel sets only the transparency. (defaults to true)
    
*   Button (buttonactions): Configures what to do when a button is pressed.
    

* * *

###### 13.3.1.1.1.6. Border

Theming of the borders.

Focused: borders for focused windows.

UnFocused: borders for unfocused windows.

*   TopLeft (texture): Texture for the top left corner.
    
*   Top (texture): Texture for the top border.
    
*   TopRight (texture): Texture for the top right corner.
    
*   Left (texture): Texture for the left border.
    
*   Right (texture): Texture for the right birder.
    
*   BottomLeft (texture): Texture for the bottom left corner.
    
*   Bottom (texture): Texture for the bottom border.
    
*   BottomRight (texture): Texture for the bottom right border.
    

* * *

#### 13.3.1.2. Harbour

Enables theming of the harbour.

*   Texture, texture: Texture to use as the harbour background.
    

* * *

#### 13.3.1.3. Menu

Themes the insides of a menu window.

*   Pad (padding): How many pixels of space around an entry is reserved.
    

* * *

##### 13.3.1.3.1. State

One of Focused, Unfocused and Selected defining the appearance when the menu/submenu is focused, not focused and the menu entry currently selected.

*   Font (fontstring): What font to use.
    
*   Background (texture): A texture that starts from the top of the menu and ends on the bottom.
    
*   Item (texture): A texture that starts from the top of a menu entry and ends on the bottom of the entry.
    
*   Text (colour): Colour of text to use.
    
*   Separator (texture): Texture to use as separator (required, client menu will break if none is defined).
    
*   Arrow (texture): Texture to use for indicating submenus (you want this to be defined too).
    

* * *

#### 13.3.1.4. CmdDialog

Themes the insides of a command dialog window.

*   Font (fontstring): What font to use.
    
*   Texture (texture): Texture to use as the background.
    
*   Text (colour): Colour of text.
    
*   Pad (padding): Amount of pixels of space around font to reserve.
    

* * *

#### 13.3.1.5. Status

Themes the insides of the status window that shows up when moving windows and so on.

*   Font (fontstring): What font to use.
    

*   Texture (texture): Texture to use as the background.
    
*   Text (colour): Colour of text.
    
*   Pad (padding): Amount of pixels of space around font to reserve.
    

* * *

#### 13.3.1.6. WorkspaceIndicator

Themes the workspace indicator that shows up when switching workspace.

*   Font (fontstring): What font to use.
    
*   Background (texture): Background for the whole window.
    
*   Workspace (texture): Texture to use when rendering a workspace.
    
*   WorkspaceActive (texture): Texture to use when rendering the active workspace.
    
*   Text (colour): Colour of text.
    
*   EdgePadding (padding): Amount of pixels of space around window edges and workspaces.
    
*   WorkspacePadding (padding): Amount of pixels of space between workspaces.