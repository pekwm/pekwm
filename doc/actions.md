[< Previous (Autoproperties)](autoproperties.md) - [(Themes) Next >](themes.md)

***

Actions
=======

**Table of Contents**

1. [Keys and Mouse](#keys-and-mouse)
1. [MoveResize](#moveresize)
1. [Menu](#menu)
1. [InputDialog](#inputdialog)


Keys and Mouse
--------------

Here is the list of all possible actions and attributes. First table
shows all toggleable attributes. Toggleable attributes are controlled
using the _Set_, _Unset_ and _Toggle_ actions. Examples below.

```
 Keypress = "Mod4 s"       { Actions = "Toggle Shaded" }
 Keypress = "Mod4 m"       { Actions = "Toggle Maximized True True" }
 Keypress = "Mod4 t"       { Actions = "Set Tagged" }
 Keypress = "Mod4 Shift t" { Actions = "Unset Tagged" }
```

### Toggleable attributes

**Maximized (bool bool)**

If a frame is maximized. Two parameters, first one decides if the
frame is maximized horizontally, the second if it is maximized
vertically.

**Fullscreen**

If a frame should be fullscreen. Fullscreen frame takes over the whole
desktop ignoring any harbour or struts and becomes decorless.

**Shaded**

If a frame should be shaded (so that only the titlebar shows until
it's unset or toggled off).

**Sticky**

If a frame should be sticky so it appears on every workspace.

**AlwaysOnTop**

If frame should always be on top of other frames.

**AlwaysBelow**

If a frame should always be below other frames.

**Decor**

When used with Set/Toggle it takes an additional parameter for the
decorname that is set/toggled. Unset just removes the last
"override"-decor.

**DecorBorder**

If frame should have borders.

**DecorTitlebar**

If frame should have a titlebar.

**Iconified**

If a frame should be iconified.

**Opaque**

If the frame should be fully opaque (ie: disable the opacity setting).

**Tagged (bool)**

If a frame should swallow all new clients until unset or toggled
off. One parameter, if true new clients open in the
background. Defaults to false.

**Marked**

If a frame is marked for later attaching (with AttachMarked).

**Skip (string)**

If a frame should be ignored on specified places, string is one of

* menus
* focustoggle
* snap
    
**CfgDeny (string)**

When things to be done to this window requested by the client program
should be denied, string is one of:

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
    
**Title (string)**

Changes the clients titlebar text to string when set. Unsetting
returns the clients title text back to the client specified one.

**HarbourHidden**

If set, harbour and anything in it will be hidden from the screen.

**GlobalGrouping**

If all autogrouping should be in use or not. By default it's set, as
in autogrouping is enabled.

### Actions

**Focus**

Gives focus to a frame.

**UnFocus**

Removes focus from a frame.

**Set (one of toggleable attributes)**

Makes toggleable attributes set.

**UnSet (one of toggleable attributes)**

Unsets toggleable attributes.

**Toggle (one of toggleable attributes)**

Toggles toggleable attributes.

**MaxFill (bool bool)**

Acts much like Maximize, but considers other frames while doing
it. Instead of filling the whole screen, it only fills to the borders
of neighboring frames. Takes two parameters, first one decides if the
frame is maxfilled horizontally, the second if it should be maxfilled
vertically.

**GrowDirection (string)**

Grows the frame in one of the directions up to the edge of the
head. String is one of:

* _up_
* _down_
* _left_
* _right_

**Close**

Closes a client window.

**CloseFrame**

Closes a frame and all client windows in it.

**Kill**

Kills a client window, use if close doesn't work.

**SetGeometry (string (int|string) [honourstrut])**

Sets the geometry of a frame. The first option is an extended version
of a geometry string that XParseGeometry can parse. That is

    [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]

Both the size and position could be followed by '%' to denote percent
instead of pixels.

The second option specifies the head number that geometry should be
relative to. It could also be a string, either (whole) "Screen" or
"Current" (head). It defaults to -1 which means the geometry is
relative to the whole screen.

If HonourStrut is specified as the third option, the screen or head
geometry will have space for panels (`_NET_WM_STRUT`) removed from the
available space.

**Raise (bool)**

Raises a frame above other frames. If bool is true raises a frame and
all of the currently active clients child/parent windows above other
frames.

**Lower (bool)**

Lowers a frame under other frames. If bool is true lowers a frame and
all of the currently active clients child/parent windows under other
frames.

**ActivateOrRaise**

If the frame this action is used on is not focused, focuses it. If the
frame is focused, raises it. If used on a groups titlebar, activates
the selected client of the group.

**ActivateClientRel (int)**

Moves the focus and raises a client inside a frame relative to the
currently selected client. Int is 1 to move right, -1 to move left.

**MoveClientRel (int)**

Moves the current clients position inside the current frame. Int is 1
to move right, -1 to move left.

**ActivateClient**

Activates a client of a frame.

### Mouse Only Actions

**ActivateClientNum (int)**

Activates the #th client of a frame. Int is the client number counting
from left.

> Keys specific

**Resize (string)**

> Mouse specific, applies to frameborder only.

Resizes a frame. String is one of:

* _top_
* _bottom_
* _left_
* _right_
* _topleft_
* _topright_
* _bottomleft_
* _bottomright_

**Move**

Moves a frame.

> Mouse specific

**MoveResize**

Activates the keyboard move and resize.

> Keys specific

**GroupingDrag (bool)**

Drags windows in and out of frames, if parameter is true dragged
windows go in the background of a frame.

> Mouse-specific

**WarpToWorkspace (string)**

Makes a dragged window warp to specified workspace when a it's moved
over a screen edge. String is one:

* _next_, send to the next workspace, if on last workspace, wrap to the first one.
* _prev_, send to the previous workspace, if on first workspace, wrap to the last one.
* _left_, send to the previous workspace
* _right_, send to the next workspace
* _int_, integer is a workspace number to send to to

> ScreenEdge specific mouse binding

**MoveToHead (int)**

Moves the frame to the same relative position on another head. The
window is shrinked to fit if it is larger than the new head.

**MoveToEdge (string)**

Moves the frame to the specified place on the screen. String is one
of:

* _TopLeft_
* _TopEdge_
* _TopRight_
* _RightEdge_
* _BottomRight_
* _BottomEdge_
* _BottomLeft_
* _LeftEdge_
* _Center_
* _TopCenterEdge_
* _BottomCenterEdge_
* _LeftCenterEdge_
* _RightCenterEdge_

> Keygrabber-specific

**NextFrame (string boolean)**

Focuses the next frame. String is one of:

* _alwaysraise_, raise windows while toggling them
* _endraise_, raise the selected client
* _neverraise_, do not raise the selected client (unless it's iconified)
* _tempraise_, raise the selected client but keep the order of the other windows

If boolean is true, also goes thru iconified windows. Defaults to
false.

**PrevFrame (string boolean)**

Focuses the previous frame. String is:

* _alwaysraise_, raise windows while toggling them
* _endraise_, raise the selected client
* _neverraise_, do not raise the selected client (unless it's iconified)
* _tempraise_, raise the selected client but keep the order of the other windows

If boolean is true, also goes thru iconified windows. Defaults to
false.

**NextFrameMRU (string boolean)**

Focuses the next frame so that the last focused windows will get
switched to first. String is:

* _alwaysraise_, raise windows while toggling them
* _endraise_, raise the selected client
* _neverraise_, do not raise the selected client (unless it's iconified)
* _tempraise_, raise the selected client but keep the order of the other windows

If boolean is true, also goes thru iconified windows. Defaults to
false.

**PrevFrameMRU (string boolean)**

Focuses the previous frame so that the last focused windows will get
switched to first. String is:

* _alwaysraise_, raise windows while toggling them
* _endraise_, raise the selected client
* _neverraise_, do not raise the selected client (unless it's iconified)
* _tempraise_, raise the selected client but keep the order of the other windows
    
If boolean is true, also goes thru iconified windows. Defaults to
false.

**FocusWithSelector string (string...)**

Focus window based on the provided selectors, if a selector does not find
a window the next will be used until a window is found or all selectors have
been tried.

String is:

* _pointer_, window under pointer.
* _workspacelastfocused_, workspace last focused window (updated on switch).
* _top_, top window.
* _root_, root window.

**FocusDirectional (string bool)**

Focuses the first window on the direction specified, and optionally
raises it. Takes two options, first one is the direction and the
second specifies if the focused frame should be raised or not. Bool
defaults to True. String is one of:

* _up_
* _down_
* _left_
* _right_

**AttachMarked**

Attachs all marked clients to the current frame.

**AttachClientInNextFrame**

Attachs client to the next frame.

**AttachClientInPrevFrame**

Attachs client to the previous frame.

**FindClient (string)**

Searches the client list for a client that has a title matching the
given regex string.

**GotoClientID (string)**

Shows and focuses a client based on the Client ID given as a
parameter.

**Detach**

Detach the current client from its frame.

**SendToWorkspace (string) [keepfocus]**

Sends a frame to the specified workspace. String is one of:

* _next_, send to the next workspace, if on last workspace, wrap to the first one.
* _prev_, send to the previous workspace, if on first workspace, wrap to the last one.
* _left_, send to the previous workspace
* _right_, send to the next workspace
* _prevv_ send to the previous (vertical) workspace, if on last workspace, wrap to the first one.
* _up_, send to the previous (vertical) workspace.
* _nextv_, sed to the next (vertical) workspace, if on last workspace, wrap to the first one.
* _down_
* _last_, send to workspace you last used before the current
* _int_, integer is a workspace number to send to to

If the second argument is "keepfocus", the window remains focused in
the new workspace. Default no keepfocus.

**GotoWorkspace (string) [bool]**

Changes workspaces. String is one of:

* _left_, go to the previous workspace on current row.
* _prev_, go to the previous workspace on current row, if on first workspace, wrap to the last one on current row.
* _right_, go to the next workspace on current row.
* _next_, go to the next workspace on current row, if on last workspace, wrap to the first one on current row.
* _leftn_, go to the previous workspace.
* _prevn_, go to the previous workspace, if on first workspace, wrap to the last.
* _rightn_, go to the next workspace.
* _nextn_, go to the next workspace, if on last workspace, wrap to the first.
* _prevv_
* _up_
* _nextv_
* _down_
* _last_, go to workspace you last used before the current
* _int_, integer is a workspace number to go to

If the second argument is given, it can be used to disable the default
behaviour of focusing the previously focused window on the workspace.

**Exec (string)**

Executes a program, string is a path to an executable file.

**Reload**

Reloads pekwm configs.

> Keys specific

**Restart**

Restarts pekwm.

> Keys specific

**RestartOther**

Quits pekwm and starts the program you specify. String is a path to an
executable file.

> Keys specific

**Exit**

Exits pekwm.

**ShowCmdDialog (string)**

Shows the command dialog that can be used to input pekwm actions. If
it's a window specific action, it affects the window focused when
CmdDialog was summoned. If entered action doesn't match any valid
pekwm action, pekwm tries to Exec it as a shell command. Takes an
optional string as a parameter. This string will then be pre-filled as
the initial value of the dialog.

**ShowSearchDialog (string)**

Shows the search dialog that can be used to search for clients and
when selected the client will be activated. Takes an optional string
as a parameter. This string will then be pre-filled as the initial
value of the dialog.

**ShowMenu (string bool)**

Shows a menu. String is menu type from below list or user defined menu
name (see [Custom Menus](configuration.md#custom-menus)):

* _root_, shows your application menu
* _icon_, shows iconified windows
* _goto_, shows currently active clients
* _gotoclient_, shows all open clients
* _window_, shows a window specific menu
* _decor_, shows possible decorations in the current theme
* _attachclient_, allows to attach clients in current frame
* _attachframe_, allows to attach whole frame in current frame
* _attachclientinframe_, allows attaching current client in any other frame
* _attachframeinframe_, allows attaching current frame in any other frame

Bool is true for sticky menus, false for click to vanish. Defaults to
false.

**HideAllMenus**

Closes all pekwm menus.

**SendKey**

Send a key, possibly with modifiers, to the active window.

**WarpPointer (int int)**

Move the cursor to the given position relative to the root window.

**SetOpacity (int int)**

Sets the Focused and Unfocused opacity values for the active
window. 100 stands for fully opaque while 0 is completely transparent.

MoveResize
----------

**MoveHorizontal (int)**

Moves a frame horizontally. Int is amount of pixels and can be
negative. Int can be followed by '%' which denotes the percent of the
screen width.

> Moveresize-specific keybinding

**MoveVertical (int)**

Moves a frame vertically. Int is amount of pixels and can be
negative. Int can be followed by '%' which denotes the percent of the
screen height.

>Moveresize-specific keybinding

**ResizeHorizontal (int)**

Resizes a frame horizontally. Int is amount of pixels and can be
negative. Int can be followed by '%' which denotes the percent of the
screen width.

>Moveresize-specific keybinding

**ResizeVertical (int)**

Resizes a frame vertically. Int is amount of pixels and can be
negative. Int can be followed by '%' which denotes the percent of the
screen height.

>Moveresize-specific keybinding

**MoveSnap**

Snaps the frame to the closest frames or screenedges.

>Moveresize-specific keybinding

**Cancel**

Cancels all moveresize actions and keeps the frame how it was before
them.

>Moveresize-specific keybinding

**End**

Acknowledges the moveresize actions and moves/resizes the frame as
wished.

>Moveresize-specific keybinding

Menu
----

**NextItem**

Goes to next menu item.

>Menu-specific keybinding

**PrevItem**

Goes to previous menu item.

>Menu-specific keybinding

**GotoItem (int)**

Goes to item number int. First item in menu is 1.

>Menu-specific keybinding

**Select**

Selects the current menu item. If it's a submenu, enter the submenu.

>Menu-specific keybinding

**EnterSubmenu**

Enters a submenu.

>Menu-specific keybinding

**LeaveSubmenu**

Leaves a submenu.

>Menu-specific keybinding

InputDialog
-----------

**Insert**

Allows for the keypress to be inputted to the text field of
InputDialog. Usually used to allow any other keys than the ones used
for InputDialog.

>InputDialog-specific keybinding

**Erase**

Erases the previous character according to the cursor position.

>InputDialog-specific keybinding

**Clear**

Clears the whole InputDialog line.

>InputDialog-specific keybinding

**ClearFromCursor**

Erases all characters after the current cursor position.

>InputDialog-specific keybinding

**Exec**

Finishes input and executes the the data

**Close**

Closes an InputDialog.

>InputDialog-specific keybinding

**CursNext**

Moves InputDialog cursor one characer space to right.

>InputDialog-specific keybinding

**CursPrev**

Moves InputDialog cursor one characer space to left.

>InputDialog-specific keybinding

**CursEnd**

Moves InputDialog cursor to the end of the line.

>InputDialog-specific keybinding

**CursBegin**

Moves InputDialog cursor to the beginning of the line.

>InputDialog-specific keybinding

**HistNext**

Get next history item previously used in InputDialog.

>InputDialog-specific keybinding

**HistPrev**

Get previous history item previously used in InputDialog.

>InputDialog-specific keybinding

***

[< Previous (Autoproperties)](autoproperties.md) - [(Themes) Next >](themes.md)
