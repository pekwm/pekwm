pekwm_sreenshot 1 "February 2021" pekwm_sreenshot "User Manual"
===============================================================

# NAME
pekwm_screenshot - a simple screenshot application

# SYNOPSIS
pekwm_screenshot [--display] [output.png]

# DESCRIPTION
pekwm_screenshot is a simple screenshot application bundled together
with pekwm.

Run without any arguments pekwm_screenshot will output an image in the
current directory named pekwm_screenshot-YYMMDDTHHMMSS-WIDTHxHEIGHT.png
where WIDTH and HEIGHT correspond to the size of the display.

If a file name is given it will be used as is without any processing
supported.

# OPTIONS
**--help** Show help information.

**--display** _DISPLAY_ Connect to DISPLAY instead of DISPLAY set in environment.

**--wait** _seconds_ to wait before taking screenshot.
