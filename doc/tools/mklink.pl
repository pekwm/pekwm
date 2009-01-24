#!/usr/bin/env perl
#
# Copyright © 2009 the pekwm development team
#

use warnings;
use strict;

my $pfx = shift(@ARGV);
my $link = shift(@ARGV);
my $desc = join(' ', @ARGV);
$link =~ s+^.*fin/++g;
print("<li><a href=\"$pfx/$link\">$desc</a></li>\n");
