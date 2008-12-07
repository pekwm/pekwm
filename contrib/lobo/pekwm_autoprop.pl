#!/usr/bin/env perl

# pekwm_autoprop.pl
# Written by: Matt Hayes <nobomb@gmail.com>
#
# Generates and adds a Property block to ~/.pekwm/autoproperties.
#
# Add a menu entry or run this script manually.  You will notice
# that upon running you get a crosshair cursor, use this to click on
# the window you want autoproperties generated for.
#
# Usage:
#	Only generate:		./pekwm_autoprop.pl
#	Generate and write:	./pekwm_autoprop.pl -w

use strict;
use warnings;

my ($_pekwm_frame_decor, $_net_wm_state, $_net_wm_desktop, $wm_class, $window_id,
	$wm_window_role, $win_x, $win_y, $win_w, $win_h, $one, $two, @wininfo, $prop,
	$info, $state, @states, $autoprops_file, $write, @props, $sticky);
	
# Get xwininfo.	
@wininfo = get_xwininfo();

# Parse out window geometry info.
foreach $info (@wininfo) {
	if ($info =~ /(.+):\s+([.\w]+)/) {
		$one = $1, $two = $2;

		if ($one =~ /xwininfo: Window id/) {
			$window_id = $two;
		} elsif ($one =~ /Absolute upper-left X/) {
			$win_x = $two;
		} elsif ($one =~ /Absolute upper-left Y/) {
			$win_y = $two;
		} elsif ($one =~ /Width/) {
			$win_w = $two;
		} elsif ($one =~ /Height/) {
			$win_h = $two;
		}
	}
}

# Get xprops.
@props = get_xprops($window_id);

# Parse out the properties we need.
foreach $prop (@props) {
	if ($prop =~ /(.+) = (.+)/) {
		if ($1 eq "WM_WINDOW_ROLE(STRING)") {
			$wm_window_role = $2;
		} elsif ($1 eq "_PEKWM_FRAME_DECOR(CARDINAL)") {
			$_pekwm_frame_decor = $2;
		} elsif ($1 eq "_NET_WM_STATE(ATOM)") {
			$_net_wm_state = $2;
		} elsif ($1 eq "_NET_WM_DESKTOP(CARDINAL)") {
			$_net_wm_desktop = $2;
		} elsif ($1 eq "WM_CLASS(STRING)") {
			$wm_class = $2;
		}
	} elsif ($prop =~ /(.+): (.+)/) {
		$one = $1, $two = $2;
		
		if ($one =~ /window id \# of group leader/) {
			$window_id = $two;
		}
	}
}

$wm_class =~ /\"(.+)\", \"(.+)\"/;

# Assemble write string.
$write =	"\n// Auto-generated autoproperty:\n".
			"// ".$1." ".$2."\n";
$write .=	"Property = \"^".$1.",^".$2."\" {\n".
			"\tApplyOn = \"Start New Reload\"\n".
			"\tClientGeometry = \"".$win_w."x".$win_h."+".$win_x."+".$win_y."\"\n";

# Set decor options
if (defined($_pekwm_frame_decor)) {
	# Titlebar false
	if ($_pekwm_frame_decor == 4 || $_pekwm_frame_decor == 0) {
		$write .= "\tTitlebar = \"False\"\n";
	}
	# Border false
	if ($_pekwm_frame_decor == 2 || $_pekwm_frame_decor == 0) {
		$write .= "\tBorder = \"False\"\n";
	}
}

# Add state booleans.
if ($_net_wm_state) {
	@states = split(/, /, $_net_wm_state);
	foreach $state (@states) {
		if ($state eq "_NET_WM_STATE_STICKY") {
			$write .= "\tSticky = \"True\"\n";
			$sticky = 1;
		} elsif ($state eq "_NET_WM_STATE_SHADED") {
			$write .= "\tShaded = \"True\"\n";
		}
	}
}

# Set workspace if this window isn't sticky.
if (!$sticky) {
	$write .= "\tWorkspace = \"".($_net_wm_desktop + 1)."\"\n";
}

# Set role
if ($wm_window_role) {
	$write .= "\tRole = ".$wm_window_role."\n";
}

# End of section.
$write .= "}\n";

# Write to autoproperties or to console.
if ($ARGV[0] && $ARGV[0] eq "-w") {
	$autoprops_file = get_autoprops_file();
	$autoprops_file =~ s/~/$ENV{'HOME'}/;
	
	open(OUTFILE, ">>".$autoprops_file) or die "Could not open ~/.pekwm/autoproperties for writing!\n";
	print OUTFILE $write;
	close(OUTFILE);
	
	print "Successfully wrote autoproperty.\n";
} else {
	print $write;
}

# Find the autoprops file.
sub get_autoprops_file {
	open(OUTFILE, "<".$ENV{'PEKWM_CONFIG_FILE'}) or die "Could not open pekwm config for reading!\n";
	my $in_sec = 0;
	
	while (<OUTFILE>) {
		if (!$in_sec && $_ =~ /Files\s+\{/) {
			$in_sec = 1;
		} elsif ($in_sec) {
			if ($_ =~ /AutoProps\s+=\s+\"(.+)\"/) {
				close(OUTFILE);
				return $1;
			}
		}
	}
	
	close(OUTFILE);
	return $ENV{HOME}."/.pekwm/autoproperties";
}

# Create an xprops array.
sub get_xprops {
	my $id = shift;
	
	my $xprop = `xprop -id $id`;
	my @xprops = split(/\n/, $xprop);
	
	return @xprops;
}

# Create an xwininfo array.
sub get_xwininfo {
	my $xwininfo;
	
	if ($ENV{CLIENT_WINDOW}) {
		$xwininfo = `xwininfo -id $ENV{CLIENT_WINDOW}`;
	} else {
		$xwininfo = `xwininfo`;
	}
	
	my @xwininfos = split(/\n/, $xwininfo);
	
	return @xwininfos;
}
