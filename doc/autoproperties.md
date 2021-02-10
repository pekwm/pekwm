[< Previous (Configuration)](configuration.md) - [(Actions) Next >](actions.md)

***

Autoproperties
==============

**Table of Contents**

1. [What are Autoproperties?](#what-are-autoproperties)
1. [Basic Autoproperties Syntax](#basic-autoproperties-syntax)
1. [Advanced Autoproperties](#advanced-autoproperties)
1. [Autogrouping](#autogrouping)
1. [TypeRules](#typerules)
1. [Getting more help](#getting-more-help)

What are Autoproperties?
------------------------

"Autoproperties" is short for "Automatic Properties". This is pekwm's
way of setting certain things up for applications based on the
window's internal id. You can set up a lot of things, such as size,
iconified state, start location, grouped state (automatically having
one window group to another), workspace to start on, whether it has a
border or titlebar, and more. It is also possible to automatically
modify window titles and to decide the order of applications on the
harbour with autoproperties.

Basic Autoproperties Syntax
---------------------------

The ~/.pekwm/autoproperties file follows the rules in [Common
Syntax](#common-syntax). This file can become rather complicated, but
it's also the most powerful of any of pekwm's config files.

The one important thing to remember is the Property tag. This
identifier tells us where to apply properties. It means which windows
to apply it on. To find out the two terms, use **xprop WM\_CLASS** and
click on your window. Below you'll find a bash/zsh function which will
give you the correct string for this file. You can also specify a
regexp wildcard, such as ".\*,opera", which means anything for the
first word, opera for the second.

```
propstring () {
  echo -n 'Property '
  xprop WM_CLASS | sed 's/.*"\(.*\)", "\(.*\)".*/= "\1,\2" {/g'
  echo '}'
}
```

Autoproperties have an both an old and new style matching clients. The
new style was introduced to support using configuration template
overwriting.

In addition with WM\_CLASS, pekwm also can identify clients by their
title string (**xprop WM\_NAME** or **xprop \_NET\_WM\_NAME**).

```
# New syntax, requires Require { Templates = "True" }
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
```

Or by their role (**xprop WM\_WINDOW\_ROLE**):

```
# New syntax, requires Require { Templates = "True" }
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
```

pekwm can rewrite window titles. This is done in a separate TitleRules
section, where one defines properties on which clients to use the
rewriting and then a regexp rule of what to do to that clients
title. These rules do not affect the actual WM\_NAME string. You can
use Role and Title keywords to narrow down the clients the titlerule
applies to. A simple rule that would change "Title: this is the title"
to "this is the title" looks like this:

```
TitleRules {
	Property = "^foo,^bar" {
		Rule = "/Title: (.*)/\\1/"
	}
}
```

In pekwm, you can make certain windows have their own decoration
set. The different decorations are defined in the theme, and they are
connected to client windows with an autoproperty. These autoproperties
reside in their own DecorRules subsection and look like this:

```
DecorRules {
	Property = "^foo,^bar" {
		Decor = "TERM"
	}
}
```

It's also possible to decide the order of applications that start in
the harbour. As with TitleRules and DecorRules, there is it's own
separate section for this purpose called Harbour. Position is a signed
int and order goes: "1 2 3 0 0 0 -3 -2 -1", and so on. That looked
cryptic. Worry not. Basically, a Position number of 0 means the
application will be placed in the middle. If the number is positive,
the application will be placed before the zero-positioned
applications. If the number is negative, the applications will be
placed after the zero-position ones. So the positive numbered show up
first in your harbour, then the zero numbered, and after the zeros
come the negatively numbered applications. I hope that is clear, the
next part is tricky.

The larger the value of the base number the
closer to the zero applications they will be. So the smaller the base
number the closer to the ends of the harbour the application will
be. Position 1 would be the first application to show up on the
harbour. And similarly Position -1 would be the last application on
the harbour. If you have application on the harbour that do not match
any of the property rules on the Harbour section, they will act as if
you had given them Position 0. Applications with the same Position
will show up next to each other in the order they are launched. In our
example below, obpager will always be placed the last on the harbour.

```
Harbour {
	Property = "^obpager,^obpager" {
		Position = "-1";
	}
}
```

If you want certain autoproperties to be only applied when you are on
a specific workspace, you can add a workspace section. The following
example sets an autoproperty that removes the border and titlebar from
xterm on the second and third workspace. Please keep in mind that we
start counting with 0.

```
Workspace = "1 2" {
    Property = "xterm,XTerm" {
        ApplyOn = "Start New Reload"
        Border = "False"
        Titlebar = "False"
    }
}
```

Here's an example ~/.pekwm/autoproperties file:

```
Property = ".*,^xmms" {
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
		Rule = "/Dillo: (.*)/\1 [dillo]/"
	}
	Property = "^opera,^opera" {
		Rule = "/...:... - (.*) - Opera .*/\\1 [opera]/"
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
```

> **Regular Expressions!**
> 
> The pekwm autoproperties file uses Regular Expression syntax for
> wildcards. Regular expressions can be really confusing to people
> with no experience with them. A good rule of thumb is: "Anywhere
> you'd think to use '\*', use '.\*'". Also, '^' matches the beginning
> of a string, '$' matches the end, and '.' is any single
> character. Pekwm has some special flags to that modifies regular
> expression matching. Specifying regular expressions in the form
> /pattern/flags allow flags to be set. The supported flags are ! for
> inverting the match and i for case insensitive matches. Explaining
> the syntax of regular expressions goes beyond the scope of this
> documentation. You might want to look at
> [http://www.regularexpressions.info/](http://www.regularexpressions.info/)
> or
> [http://en.wikipedia.org/wiki/Regular\_expressions](http://en.wikipedia.org/wiki/Regular_expressions).

Advanced Autoproperties
-----------------------

Below is a list of the different actions available to you in your
autoproperties file; These are the actual Auto Properties. They can
take four types of arguments:

| Type    | Description                                                                                                   | Example     |
|---------|---------------------------------------------------------------------------------------------------------------|-------------|
| boolean | Is either True (1) or False (0)                                                                               | true        |
| integer | An Integer is a number, negative or positive                                                                  | 42          |
| string  | A string is any string, it's used as an identifier                                                            | MENU        |
| geom    | X Geometry String by the form "\[=\]\[<width>\[%\]{xX}<height>\[%\]\]\[{+-}<xoffset>\[%\]{+-}<yoffset>\[%\]\] | 200x100+0+0 |

**Exhaustive Autoprops List**

**AllowedActions (string)**
**DisallowedActions (string)**

A list of actions to allow/deny performing on a client:

* _Move_, (Dis)allow moving of the client window
* _Resize_, (Dis)allow resizing of the client window
* _Iconify_, (Dis)allow iconifying of the client window
* _Shade_, (Dis)allow shading of the client window
* _Stick_, (Dis)allow setting sticky state on the client window
* _MaximizeHorizontal_, (Dis)allow maximizing the client window horizontally
* _MaximizeVertical_, (Dis)allow maximizing the client window vertically
* _Fullscreen_, (Dis)allow setting the client window in fullscreen mode
* _SetWorkspace_, (Dis)allow changing of workspace
* _Close_, (Dis)allow closing
    
**ApplyOn (string)**

A list of conditions of when to apply this autoprop (so be sure to
include this in your property), consisting of:

* _New_, applies when the application first starts)
* _Reload_, apply when pekwm's config files are reloaded)
* _Start_, apply if window already exists before pekwm starts/restarts. Note when using grouping Start will not take workspaces in account)
* _Transient_, apply to Transient windows as well as normal windows. Dialog boxes are commonly transient windows
* _TransientOnly_, apply to Transient windows only. Dialog boxes are commonly transient windows
* _Workspace_, apply when the window is sent to another workspace

**Border (bool)**

Window starts with a border

**CfgDeny (string)**

A list of conditions of when to deny things requested by the client
program, consisting of

* _Above_, ignore client request to always place window above other windows)
* _ActiveWindow_, ignore client requests for showing and giving input focus)
* _Below_, ignore client request to always place window below other windows)
* _Fullscreen_, ignore client request to set window to fullscreen mode)
* _Hidden_, ignore client request to show/hide window)
* _MaximizedHorz_, ignore client request to maximize window horizontally)
* _MaximizedVert_, ignore client request to maximize window vertically)
* _Position_, ignore client requested changes to window position)
* _ResizeInc_, ignore resize increments from the size hints. Use on terminals to make them fill up all available space.
* _Size_, ignore client requested changes to window size)
* _Stacking_, ignore client requested changes to window stacking)
* _Strut_, ignore client request for reserving space in the screen corners, typically done by panels and the like)

**ClientGeometry (geom)**

X Geometry String showing the initial size and position of the client,
excluding the possible pekwm titlebar and window borders.

**Decor (string)**

Use the specified decor for this window. The decor has to be defined
in the used theme. The decor is chosen by the first match in order:
AutoProperty, TypeRules, DecorRules.

**Focusable (bool)**

Toggles if this client can be focused while it's running.

**FocusNew (bool)**

Toggles if this client gets focused when it initially pops up a
window.

**FrameGeometry (geom)**

X Geometry String showing the initial size and position of the window
frame. Window frame includes the client window and the possible pekwm
titlebar and window borders. If both ClientGeometry and FrameGeometry
are present, FrameGeometry overrides the ClientGeometry.

**Fullscreen (bool)**

Window starts in fullscreen mode

**Group (string)**

Defines the name of the group. Also the section that contains all the
grouping options. They are:

* _Behind (bool)_, if true makes new clients of a group not to become the active one in the group.
* _FocusedFirst (bool)_, if true and there are more than one frame where the window could be autogrouped into, the currently focused frame is considered the first option.
* _Global (bool)_, if true makes new clients start in a group even if the group is on another workspace or iconified.
* _Raise (bool)_, if true makes new clients raise the frame they open in.
* _Size (integer), how many clients should be grouped in one group.

**Iconified (bool)**

Window starts Iconified

**Layer (string)**

Windows layer. Makes the window stay under or above other
windows. Default layer is "Normal". Possible parameters are (listed
from the bottommost to the uppermost):

* _Desktop_
* _Below_
* _Normal_
* _OnTop_
* _Harbour_
* _AboveHarbour_
* _Menu_

**MaximizedHorizontal (bool)**

Window starts Maximized Horizontally

**MaximizedVertical (bool)**

Window starts Maximized Vertically

**Opacity (int int)**

Sets the focused and unfocused opacity values for the window. A value
of 100 means completely opaque, while 0 stands for completely
transparent.

Note that a Composite Manager needs to be running for this feature to
take effect.

**PlaceNew (bool)**

Toggles the use of placing rules for this client.

**Role (string)**

Apply this autoproperty on clients that have a WM\_WINDOW\_ROLE hint
that matches this string. String is a regexp like: "^Main".

**Shaded (bool)**

Window starts Shaded

**Skip (string)**

A list of situations when to ignore the defined application and let
the user action skip over it, consisting of

* _Snap_, do not snap to this window while moving windows)
* _Menus_, do not show this window in pekwm menus other than the icon menu)
* _FocusToggle_, do not focus to this window when doing Next/PrevFrame)
    
**Sticky (bool)**

Window starts Sticky (present on all workspaces)

**Title (string)**

Apply this autoproperty on clients that have a title that matches this
string. String is a regexp like: "^Saving".

**Titlebar (bool)**

Window starts with a TitleBar

**Workspace (integer)**

Which workspace to start program on.

AutoGrouping
------------

Autogrouping is actually very simple, although it might be a bit
confusing at first. Group is an identifier, it's just a string, (in my
example, we'll call it netwin). Size tells how many clients to group
together in one frame.

The example: We want to autogroup Sylpheed and Opera together,
allowing as many instances of the program windows to be grouped as
there are. Here's the Autoprops section for that:

```
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
```

This creates two rules: "For any window matching '.\*,^opera', group
these in the 'netwin' group. Apply this on pekwm start/reload and when
new windows matching this property are opened, but do not include
dialog windows", and "For any window matching '.\*,^Sylpheed', group
in the 'netwin' group. Apply on pekwm start/reload and when new
windows matching this property are opened, also include possible
dialog windows to the group. Open the window to the group but do not
bring it upmost automatically".

To group unlimited windows together, use size 0.

Also note that you can have as many Group identifiers as you
want. Autogrouping is a very flexible system. Try playing around with
it.

TypeRules
---------

The TypeRules decides how the \_NET\_WM\_WINDOW\_TYPE should be
interpreted. The \_NET\_WM\_WINDOW\_TYPE hint gives the application
writer possibility to inform the window manager what kind of window it
is creating.

TypeRules are defined in the TypeRules section of the
~/.pekwm/autoproperties file. A sample section could look something
like this:

```
TypeRules {
    ...

    Property = "MENU"  {
        Titlebar = "False"
        Border = "False"
        Skip = "FocusToggle Menus Snap"
    }

    ...
}
```

Using TypeRules are done the same way as with [Advanced
Autoproperrties](#advanced-autoproperties) but the property is matched
based on the value of \_NET\_WM\_WINDOW\_TYPE. Supported values are
available in the list below.

**Supported values**

* _Desktop_, A desktop window such as the window containing desktop icons on the Gnome desktop.
* _Dock_
* _Toolbar_
* _Menu_
* _Utility_
* _Splash_, Application startup screen usually presenting loading progress.
* _Dialog_, Dialogs prompting for information such as "Save as" dialogs.
* _Normal_, Any other window, can be used to set default autoproperties.

Getting more help
-----------------

Autoprops can be a daunting topic. If you've read everything here in
the docs and are still having problems, feel free to hit the
[IRC](development#irc) channel and ask. Check the [Common questions
and answers](faq.md) before asking. Remember that: "IF YOU WANT AN
ANSWER TO YOUR QUESTION, YOU HAD BETTER HAVE ALREADY READ THE
DOCUMENTATION".

***

[< Previous (Configuration)](configuration.md) - [(Actions) Next >](actions.md)
