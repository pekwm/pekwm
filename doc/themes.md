[< Previous (Actions)](actions.md) - [(FAQ) Next >](faq.md)

***

Themes
======

This section aims to documenting the pekwm theme structure. It's
rather cryptic at first look, sorry.

Please use existing themes as real life examples and base when it
comes to making your own.

Guidelines
----------

It is strongly recommended and expected that theme tarballs are
labeled for the pekwm version they are made and tested with. The
filename format should be
theme\_name-pekwm\_version.\[tar.gz|tgz|tar.bz2|tbz\]. For example
silly\_theme-pekwm\_0.1.5.tar.bz2.

It is also highly recommended that theme directories are named in a
similar fashion. However, for stable releases this is not mandatory,
the tarball filename is enough. If you're building for a GIT revision,
mention it in as many places as possible.

The silly theme from above would contain a directory structure as
follows:

```
silly_theme-pekwm_0.1.5/
pekwm_0.1.5/theme
pekwm_0.1.5/menubg.png
pekwm_0.1.5/submenu.png
```

The theme file header should contain at least the themes name, the
pekwm version the theme is for, address to reach the theme
maker/porter or get an updated theme, and a last modified
date. Changelog entries won't hurt if you aren't the original theme
author. For example:

```
# silly, a PekWM 0.1.5 theme by shared (themes@adresh.com)
# This theme is available from hewphoria.com.
# Last modified 20060529.

# Extract this theme directory under ~/.pekwm/themes/ and the
# themes menu will pick it up automatically.

# Changelog:
# 2006-05-29 HAX0ROFUNIVERSE <hawt@haxorland.invalid>
#  * REWROTE EVERYTHING WITH CAPS LOCK ON,
#    CAPS LOCK IS CRUISE CONTROL FOR COOL!
```

Try to stick to the theme syntax and rather than deleting entries
please use the EMPTY texture.

Attributes
----------

Here is the explanation of Attributes names of themes

| Attribute     | Description                                                                                                                                                                    | Example                                 |
|---------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------|
| pixels        | An integer, amount of pixels.                                                                                                                                                  | example: "2"                            |
| size          | Pixels vertically times pixels horizontally.                                                                                                                                   | example: "2x2"                          |
| percent       | Any percent value from 1 to 100.                                                                                                                                               | example: "87"                           |
| toggle        | Sets a value as true (1) or false (0).                                                                                                                                         | example: "true"                         |
| padding       | Free pixels from top, free pixels under, free pixels from left, free pixels from right.                                                                                        | example: "2 2 2 2"                      |
| decorname     | Name for decoration                                                                                                                                                            | example: "Default"                      |
| colour        | A colour value in RGB format.                                                                                                                                                  | example: "#FFFFFF"                      |
| imagename     | Name of the imagefile with an option after the #                                                                                                                               | example: "image.png"                    |
| texture       | Any valid texture                                                                                                                                                              | example: "Solid #ffffff 4x4             |
| fontstring    | Defines a font. Chopped to parts by # marks. First the font type (XFT or X11), then the font name, then the text orientation, then shadow offsets. Some fields can be omitted. | example: "XFT#Verdana:size=10#Left#1 1" |
| buttonactions | Buttonactions work alike what you are used from the mouse config, first mouse button number pressed when this action should happen, then any standard pekwm actions.           | example: "1" { Actions = "Close" }      |

### Decor names

Any name can be used and applied to windows with autoproperties or the
set decor action. The list below includes names with special meaning:

* _DEFAULT_, Defines decorations to all windows unless overridden with another decoration set (REQUIRED).
* _ATTENTION_, Defines the decoration for windows that set the urgency/demand-attention hint.
* _BORDERLESS_, Defines decorations for borderless windows (recommended).
* _INPUTDIALOG_, Defines decorations for input dialogs, such as the CmdDialog.
* _MENU_, Defines decorations for menus.
* _STATUSWINDOW_, Defines decorations for the command dialog.
* _TITLEBARLESS_, Defines decorations for titlebarless windows (recommended, should be there if your theme looks nasty when toggled titlebarless).
* _WORKSPACEINDICATOR_, Defines decorations for the workspace indicator.

### Image options

* _#fixed_, Image is fixed size.
* _#scaled_, Image will be scaled to fit the area it's defined for.
* _#tiled_, Image will be repeated as many times as needed to fill the area it's defined for. This is the default if no option is specified.
    
### Textures

* _EMPTY_, No texture (transparent).
* _SOLID colour size_, A solid colour texture of defined colour and size.
* _SOLIDRAISED colour colour colour pixels pixels toggle toggle toggle toggle size_, A solid colour texture with a 3D look of defined colours, form and size. First colour defines the main fill colour, second the highlight colour used on the left and top parts of the texture, third the highlight colour on the bottom and right parts of the texture. First pixel amount defines how fart apart the 3D effects are from each other, second pixel amount is how thick the bordering will be (both pixels default to 1). The four toggles are used to tell which raised corners are to be drawn. This is useful for example when defining solidraised frame corner pieces. The order is Top, Bottom, Left, Right (not unlike that used in padding). As example: "True False True False" (or 1 0 1 0) could mean you want to draw the TopLeft piece of a solidraised window border. Size should explain itself, see above.
* _IMAGE imagename_, An image texture using the defined imagename

Theme structure
---------------

### PDecor

The block for decoration sets, any amount of Decor sections can exist
inside this block.

#### Decor

A list of blocks with theme specifications the various types of
decorations.

##### Title

Theming of the frame.

* _Height (pixels)_, Amount of pixels the titlebar should height.
* _HeightAdapt (boolean)_, If true, Height is adapted to fit the Title font.
* _Pad (pixels t,l,r,b)_, How many pixels are left around a title text.
* _Focused (texture)_, Background texture for a focused titlebar.
* _UnFocused (texture)_, Background texture for an unfocused titlebar.
* _WidthMin (pixels)_, Minimum width of title in pixels, will also place the titlebar outside of the window borders. Use 0 to place titlebar inside borders.
* _WidthMax (percent)_, Maximum width of titles relative to window width, when this value ends up being smaller than the value in WidthMin, WidthMin is overridden.
* _WidthSymetric (boolean)_, Set true to constant width titles or false to use titles that only are as big as the clients title text string requires (note, asymmetric width is not fully implemented yet, always set this true for now to avoid problems).
    
##### Tab

Theming of a titlebar tabs.

* _Focused (texture)_, Background texture for a tab of a focused window.
* _Unfocused (texture)_, Background texture for a tab of an unfocused window.
* _FocusedSelected (texture)_, Background texture for the currently selected tab of a focused window.
* _UnFocusedSelected (texture)_, Background texture for the currently selected tab of an unfocused window.
    
##### FontColor

Theming of font colors.

* _Focused (colour colour)_, Text colour for a tab of a focused window. second value is the shadow colour.
* _Unfocused (colour colour)_, Text colour for a tab of an unfocused window. second value for shadow.
* _FocusedSelected (colour colour)_, Text colour for the currently selected tab of a focused window. second value for shadow.
* _UnFocusedSelected (colour colour)_, Text colour for the currently selected tab of an unfocused window. second value for shadow.

##### Font

Theming of the titlebar fonts.

* _Focused (fontstring)_, Font of the text of a tab of a focused window.
* _Unfocused (fontstring)_, Font of the text of a tab of an unfocused window.
* _FocusedSelected (fontstring)_, Font of the text of the currently selected tab of a focused window.
* _UnFocusedSelected (fontstring)_, Font of the text of the currently selected tab of an unfocused window.
    
##### Separator

Theming of the tab separator.

* _Focused (texture)_, Separator texture for a focused window.
* _Unfocused (texture)_, Separator texture for an unfocused window.
    
##### Buttons

Theming of titlebar buttons.

```
Right = "Name"
```

Places the button on the right end of the titlebar.

```
Left = "Name"
```

Places the button on the left end of the titlebar.

* _Focused (texture)_, Texture for button of a focused window.
* _Unfocused (texture)_, Texture for button of an unfocused window.
* _Pressed (texture)_, Texture for button that is pressed.
* _Hover (texture)_, Texture for button when pointer is placed on it.
* _SetShape (bool)_, If true, the shape of the button is derived from the alpha-channel. If false, the alpha-channel sets only the transparency. (defaults to true)
* _Button (buttonactions)), Configures what to do when a button is pressed.
    
##### Border

Theming of the borders.

Focused: borders for focused windows.

UnFocused: borders for unfocused windows.

* _TopLeft (texture)_, Texture for the top left corner.
* _Top (texture)_, Texture for the top border.
* _TopRight (texture)_, Texture for the top right corner.
* _Left (texture)_, Texture for the left border.
* _Right (texture)_, Texture for the right birder.
* _BottomLeft (texture)_, Texture for the bottom left corner.
* _Bottom (texture)_, Texture for the bottom border.
* _BottomRight (texture)_, Texture for the bottom right border.
    
##### Harbour

Enables theming of the harbour.

* _Texture (texture)_, Texture to use as the harbour background.
    
##### Menu

Themes the insides of a menu window.

* _Pad (padding)_, How many pixels of space around an entry is reserved.
    
##### State

One of Focused, Unfocused and Selected defining the appearance when
the menu/submenu is focused, not focused and the menu entry currently
selected.

* _Font (fontstring)_, What font to use.
* _Background (texture)_, A texture that starts from the top of the menu and ends on the bottom.
* _Item (texture)_, A texture that starts from the top of a menu entry and ends on the bottom of the entry.
* _Text (colour)_, Colour of text to use.
* _Separator (texture)_, Texture to use as separator (required, client menu will break if none is defined).
* _Arrow (texture)_, Texture to use for indicating submenus (you want this to be defined too).

##### CmdDialog

Themes the insides of a command dialog window.

* _Font (fontstring)_, What font to use.
* _Texture (texture)_, Texture to use as the background.
* _Text (colour)_, Colour of text.
* _Pad (padding)_, Amount of pixels of space around font to reserve.

##### Status

Themes the insides of the status window that shows up when moving
windows and so on.

* _Font (fontstring)_, What font to use.
* _Texture (texture)_, Texture to use as the background.
* _Text (colour)_, Colour of text.
* _Pad (padding)_, Amount of pixels of space around font to reserve.

##### WorkspaceIndicator

Themes the workspace indicator that shows up when switching workspace.

* _Font (fontstring)_, What font to use.
* _Background (texture)_, Background for the whole window.
* _Workspace (texture)_, Texture to use when rendering a workspace.
* _WorkspaceActive (texture)_, Texture to use when rendering the active workspace.
* _Text (colour)_ Colour of text.
* _EdgePadding (padding)_, Amount of pixels of space around window edges and workspaces.
* _WorkspacePadding (padding)_, Amount of pixels of space between workspaces.

***

[< Previous (Actions)](actions.md) - [(FAQ) Next >](faq.md)
