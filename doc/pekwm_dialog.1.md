pekwm_dialog 1 "February 2021" pekwm_dialog "User Manual"
=========================================================

# NAME
pekwm_dialog - a xmessage inspired dialog application

# SYNOPSIS
pekwm_dialog [--display] message

# DESCRIPTION

pekwm_dialog presents a simple dialog with a title, image, message
text and a button for each options specified. If no options are
specified the dialog will have an Ok button.

The caller can identify which options was selected by checking the
exit code of the application. The first option will exit with status
code 0, next with 1 and so on.

Given this behavior it is recommended to have ok as the first option
and then cancel as the second and then further options. This caters
for standard exit code 0 on ok, and cancel would give the same exit
code as on errors.

The illustration below depicts the layout of dialog elements:

```
--------------------------------------------
| TITLE                                    |
--------------------------------------------
|                                          |
| Image if any is displayed here           |
|                                          |
--------------------------------------------
| Message text goes here                   |
|                                          |
--------------------------------------------
|           [Option1] [Option2]            |
--------------------------------------------
```

# OPTIONS
**--help** Show help information.

**--display** _DISPLAY_ Connect to DISPLAY instead of DISPLAY set in environment.

**--decorations** Set requested decorations. all, no-border and no-titlebar supported.

**--geometry** _GEOMETRY_ Geometry of window, default is 200x50 expanding for image size.

**--image** _IMAGE_ Path to image to display.

**--option** _STRING_ Option title, multiple options are supported.

**--title** _STRING_ Title string.
