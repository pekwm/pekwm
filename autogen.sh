#!/bin/sh
#
# As which and whereis behave different on various platforms, a simple
# shell routine searching the split PATH is used instead.

# Do this outside the routine for effiency
PATH_SPLIT=`echo $PATH | awk -F ':' '{ for (i = 1; i < NF; i++) print $i}'`

# Search for command in PATH_SPLIT, sets command_found
find_command()
{
  command_found=''
  for dir in $PATH_SPLIT; do
    if [ -x "$dir/$1" ]; then
      command_found="$dir/$1"
      break
    fi
  done
}

# Search fod command, print $1 and run command with arg
find_fallback_and_execute()
{
 find_command "$2"
 if [ -z "$command_found" ]; then
   command_found="$3"
 fi

 printf " $1"
 $command_found $4
}

# Begin output
echo "Generating build scripts, this might take a while."

find_fallback_and_execute "aclocal" "aclocal" "aclocal-1.10" ""
find_fallback_and_execute "autoheader" "autoheader" "autoheader-2.59" ""
find_fallback_and_execute "autoconf" "autoconf" "autoconf-2.59" ""
find_fallback_and_execute "automake" "automake" "automake-1.10" "-a"

# End output
echo ""
echo "Done generating build scripts."
