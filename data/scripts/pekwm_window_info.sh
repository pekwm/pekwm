#!/bin/sh
# Copyright © 2005 by the pekwm development team
#
# Shows some info of the currently active client.
# Add this to your menu:
#
# SubMenu = "Themes" {
#   Entry = "" { Actions = "Dynamic /path/to/this/file" }
# }
#

ID=`xprop -root _NET_ACTIVE_WINDOW | cut -f2 -d#`
CLASS=`xprop -id $ID WM_CLASS | cut -d\" -f2,4 --output-delimiter ,`
NAME=`xprop -id $ID WM_NAME | cut -d\" -f2 | sed 's/\"/\\\"/'`
ROLE=`xprop -id $ID WM_WINDOW_ROLE | cut -d\" -f2`

echo Dynamic {
# echo "Entry = \"WindowID = $ID\" { Actions = \"Exec xmessage $ID\" }"
echo "Entry = \"Property = \\\"$CLASS\\\"\" { Actions = \"Exec xmessage Property = \\\\\"$CLASS\\\\\"\" }"
echo "Entry = \"Title = \\\"$NAME\\\"\" { Actions = \"Exec xmessage Title = \\\\\"$NAME\\\\\"\" }"
echo "Entry = \"Role = \\\"$ROLE\\\"\" { Actions = \"Exec xmessage Role = \\\\\"$ROLE\\\\\"\" }"
echo }

# Note, xmessage output relies on a parser bug. Remove one pair of \'s
# once it's fixed. --shared

