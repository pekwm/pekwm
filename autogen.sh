#!/bin/sh

echo "Generating configure script, this will take a while..."

echo -n "	aclocal"
cmd_aclocal=`which aclocal-1.9`
if [ -z $cmd_aclocal  ] || [ ! -x $cmd_aclocal ]; then
  cmd_aclocal="aclocal"
fi
$cmd_aclocal

echo -n " autoheader"
cmd_autoheader=`which autoheader-2.59`
if [ -z $cmd_autoheader ] || [ ! -x $cmd_autoheader ]; then
  cmd_autoheader="autoheader"
fi
$cmd_autoheader

echo -n " autoconf"
cmd_autoconf=`which autoconf-2.59`
if [ -z $cmd_autoconf ] || [ ! -x $cmd_autoconf ]; then
  cmd_autoconf="autoconf"
fi
$cmd_autoconf

echo " automake"
cmd_automake=`which automake-1.9`
if [ -z $cmd_automake ] || [ ! -x $cmd_automake ]; then
  cmd_automake="automake"
fi
$cmd_automake -a
