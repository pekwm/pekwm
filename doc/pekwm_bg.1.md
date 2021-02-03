pekwm_bg 1 "February 2021" pekwm_bg "User Manual"
=================================================

# NAME
pekwm_bg - a simple background setting application

# SYNOPSIS
pekwm_bg [--display] texture

# DESCRIPTION
pekwm_bg is a simple background setting application bundled together
with pekwm and is typically run whenever a theme has a background
texture specified.

pekwm_bg repeats the texture on each head, making images appear once
for every available head.

pekwm_bg accepts textures in the same format as pekwm themes where the
most common ones for backgrounds are:

* **Image** background.png#scaled, background.png scaled to screen size.
* **Solid** #eeeeee, solid color.
* **LinesHorz** 33% #afadbf #9f9daf #afadbf, 3 horizontal lines.

# OPTIONS
**--daemon**, run as daemon.

**--display** _DISPLAY_ Connect to DISPLAY instead of DISPLAY set in environment.

**--help** Show help information.

**--load-dir** DIR Load images from specified directory.

**--stop** Stop running pekwm_bg daemon.
