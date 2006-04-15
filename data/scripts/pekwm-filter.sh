#!/bin/sh

# Written by Aristotle Pagaltzis <pagaltzis@gmx.de>
# This code is in the public domain.
# It is provided as is, without warranty of any kind.
# Do with it whatever you want. If it breaks, you can keep both parts.

# Some automation changes by Jyri Jokinen <shared@adresh.com> made so
# that users can run this from the pekwm menu.

if [ $# -eq 0 ] ; then cat <<END_HELP ; exit 1 ; fi
usage: `basename $0` menufile

  Replaces the given pekwm menu file, filtered to exclude "Exec" menu
  entries which refer to nonexistant binaries. The following simple
  forms of "Actions" directives are supported:

      Exec binary --foo --bar
      Exec \$TERM -foo bar -baz quux -e binary --qux -wibble wobble

  In these cases, `basename $0` will only emit the line if 'binary' can
  be found in \$PATH.

  Further it supports a more complex variable form:

      \$FOO_BIN = "foo"
      \$FOO_BIN = "foo2"
      \$FOO_BIN = "superfoo"
      Exec \$FOO_BIN

  In these cases, `basename $0` will filter the variable assignments as
  above, but will only emit the 'Exec' line if any one of the variables
  was previously emitted. Note that variables must have a _BIN suffix to
  be filtered.

  Note that only one action per line is supported.

END_HELP

cp "$@" "$@.backup"

sed -f - "$@.backup" <<'END_SED' > "$@.sh" | sh
s/'/'\\''/g;
s/.*$\([^ ;]*_BIN\) *= *" *\([^ ;"]*\).*/if which \2 \&> \/dev\/null ; then \1=1 ; echo '&' ; fi/; t;
s/.*[Ee]xec \+$TERM\( \+\([^-;"][^ ;"]\+\|-[^e][^ ;"]*\)\)* \+-e \+\([^ ;"]\+\).*/if which \3 \&> \/dev\/null ; then echo '&' ; fi/; t;
s/.*[Ee]xec \+\([^$ ;"][^ ;"]*\).*/if which \1 \&> \/dev\/null ; then echo '&' ; fi/; t;
s/.*[Ee]xec \+\($[^ ;"]*_BIN\) .*/if [ "\1" ] ; then echo '&' ; fi/; t;
s/.*/echo '&'/;
END_SED

/bin/sh "$@.sh" > "$@"
rm "$@.sh"

pkill -HUP pekwm

