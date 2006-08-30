#!/usr/bin/env perl
#
# Copyright  © 2003 by the pekwm development team
#

use warnings "all";
use strict;

my @args = qw(send goto);
die "Please specify one of '", join("', '", @args), "'.\n" if($#ARGV != 0);

my $type = $ARGV[0] =~ /send/i ? "Send" : "Goto";

my $ws = ${[split(/\s+/, `xprop -root _NET_NUMBER_OF_DESKTOPS`)]}[2];

print "Dynamic {\n";

for (my $i = 1; $i <= $ws; $i++) {
	print "Entry = \"Workspace $i\" { Actions = \"${type}Workspace $i\" }\n";
}

print "}\n";
