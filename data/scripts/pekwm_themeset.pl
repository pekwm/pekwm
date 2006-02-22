#!/usr/bin/env perl
#
# Copyright © 2003 by the pekwm development team
#
# Add this to your menu to use this script:
#
# SubMenu = "Themes" {
#   Entry = "" { Actions = "Dynamic /path/to/this/file /path/to/themedir" }
# }
#

use warnings;
use strict;

sub menu {
	my ( $dir ) = @_;

	my @themes = (
		sort
		grep { !/^\./ and $_ ne 'CVS' and -d( "$dir/$_" ) }
		do {
			opendir my $dh, $dir
				or die "Can't read directory $dir: $!\n";
			readdir $dh;
		},
	);

	print(
		"Dynamic {\n",
		map( qq(Entry = "$_" { Actions = "Exec $0 set $dir/$_" }\n), @themes ),
		"}\n"
	);
}

sub set_theme {
	my ( $theme ) = @_;
	my $cfg_file = $ENV{PEKWM_CONFIG_FILE};

	my $config = do {
		open my $fh, '<', $cfg_file
			or die "Can't open $cfg_file for reading: $!\n";
		local $/; # slurp file
		<$fh>
	};

	$config =~ s/Theme\s*=\s*".*?"/Theme = "$theme"/i;

	open my $fh, '>', $cfg_file
		or die "Can't open $cfg_file for writing: $!\n";
	syswrite $fh, $config;
	close $fh
		or die "Failed to write $cfg_file successfully: $!\n";

	system 'pkill', -HUP => 'pekwm'; # tell pekwm to reread its config
}

if( @ARGV == 1 ) {
	menu( $ARGV[ 0 ] );
}
elsif( @ARGV == 2 and $ARGV[ 0 ] eq 'set' ) {
	set_theme( $ARGV[ 1 ] );
}
else {
	print STDERR (
		"usage:\n",
		"\t$0 <themedir>\n",
		"\t$0 set <themename>\n",
	);
}

