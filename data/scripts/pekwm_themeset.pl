#!/usr/bin/perl
#
# Copyright  © 2003 by the pekwm development team
#
# Add this to your menu, if you have pekwm's dynamic menu support:
#
# SubMenu = "Themes" {
#   Entry = "" { Actions = "Dynamic /path/to/this/file /path/to/themedir" }
# }
#

use warnings "all";
use strict;

if (scalar(@ARGV) == 1) { # Specifying a directory
  my $dir = $ARGV[0];

	opendir(DIR, "$dir") || die "Can't opendir $dir: $!";
	my @themes = grep { (! /^\./) && (-d "$dir/$_") } readdir(DIR);
	closedir DIR; 

	print("Dynamic {\n");
	foreach my $x (@themes) {
		if ($x eq 'CVS') {
			next;
		}
		my $y = $x;
		$y =~ s+.*/++g;
		print("Entry = \"$y\" { Actions = \"Exec $0 set $dir/$x\" }\n");
	}
	print("}\n");

} elsif (scalar(@ARGV) == 2) { # Specifying a theme to set
	my $theme = $ARGV[1];

	open(PKCONF, "<$ENV{HOME}/.pekwm/config") or die "eep!";
	my @file = <PKCONF>;
	close(PKCONF);

	my @file2 = ();
	foreach (@file) { 
		s/Theme = ".*"/Theme = "$theme"/gi;
		push(@file2, $_);
	};
	open(PKCONF, ">$ENV{HOME}/.pekwm/config") or die "eep2!";
	print(PKCONF @file);
	close(PKCONF);

	system("pkill -HUP pekwm"); # Make pekwm re-read it's config
}
