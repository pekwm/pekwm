#!/usr/bin/env bash
#
# Copyright (c) 2003-2004 by the pekwm development team
#
# Add this to your menu to use this script:
#
# SubMenu = "Themes" {
#   Entry { Actions = "Dynamic /path/to/this/file /path/to/themedir" }
# }
#

# list themes
if [[ "x$2" == "x" ]]; then

    DIR=$1
    if [ -d $DIR ]; then
				echo "Dynamic {";
				for i in `ls $DIR | grep -v CVS`; do
						if [ -d $DIR/$i ]; then
								echo -e "Entry = \"$i\" { Actions = \"Exec $0 set $DIR/$i\" }";
						fi
				done;
				echo "}";

    else
				exit 1;
    fi;

# change theme
else
    
    THEME=$(echo $2 | sed -e 's/\//\\\//g');
    #if you don't have your pekwm config
    if [ ! -w $PEKWM_CONFIG_FILE ]; then 
						exit 1; #if you have pekwm installed ;)
		fi;

		if [ -x /bin/mktemp ]; then
				TMPFILE=`mktemp -t pekwm_themeset.XXXXXXXXXX` || exit 1;
		else
				TMPFILE="/tmp/pekwm_themeset.tmp"
		fi

		sed -e "s/Theme\ =\ \".*\"/Theme\ =\ \"$THEME\"/" $PEKWM_CONFIG_FILE > $TMPFILE;
		cp $TMPFILE $PEKWM_CONFIG_FILE;
		rm $TMPFILE
		pkill -HUP pekwm;
fi;

exit 0
