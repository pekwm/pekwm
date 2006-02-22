#!/usr/bin/env perl
#
# Copyright (C) 2003-2006 by the pekwm development team
#
use warnings;
use strict;

use Getopt::Std;

my %args = (
            send => 'SendToWorkspace',
            goto => 'GotoWorkspace',
            );

getopts( 'n', \my %opt );

my $type = $args{ $ARGV[0] || '' };

die "usage: $0 [ -n ] < ", join( ' | ', keys %args ), " >\n" if not $type;

my $num_workspaces = ( split / = /, `xprop -root _NET_NUMBER_OF_DESKTOPS` )[1];

print "Dynamic {\n" if not $opt{n};

for my $i ( 1 .. $num_workspaces ) {
    print qq(Entry = "Workspace $i" { Actions = "$type $i" }\n);
}

print "}\n" if not $opt{n};
