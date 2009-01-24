#!/usr/bin/env perl
#
# Copyright © 2009 the pekwm development team
#

use warnings;
use strict;

my $pfx  = shift(@ARGV);
my $link = shift(@ARGV);
my $desc = join(' ', @ARGV);
my $du   = qx(ls -lh $link | awk '{print \$5}');
$link =~ s+^.*fin/++g;
chomp($du);
# print("ls -lh fin/$link | awk {print \$5}\n");
print("<li><a href=\"$pfx/$link\">$desc ($du)</a></li>\n");
