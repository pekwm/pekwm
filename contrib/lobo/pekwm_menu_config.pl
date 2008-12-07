#!/usr/bin/env perl

# pekwm_menu_config.pl
# Written by: Matt Hayes <nobomb@gmail.com>
#
# !!! Requires package 'zenity' for the GTK2 dialogs.
# !!! Back up your config, this will rewrite it in alphabetical order.
#     Nothing usually goes wrong, but do it just to be safe.
#
# Generates a dynamic pekwm menu of all ~/.pekwm/config variables.
# For boolean values, clicking on the menu entry will toggle between
# 'True' or 'False'.  Clicking on single integer values will pop up
# a scale dialog which has a graphical slider to select a new value.
# Any other values are treated as strings and will be edited by a
# graphical entry box.
#
# Add to your menu an entry like so:
#
# Submenu = "Config" {
#   Entry { Actions = "Dynamic /path/to/pekwm_menu_config.pl --genmenu" }
# }
#
# Put check.png and uncheck.png in your icons directory (~/.pekwm/icons/).

use strict;
use warnings;
use Cwd qw(abs_path);

# Path to this script.
my $path = abs_path($0);
my $vars_path = $path.".vars";

# Config file
my $config_in = $ENV{'PEKWM_CONFIG_FILE'};
my $config_out = $config_in;

# Check command line arguments.
if ($ARGV[0] && $ARGV[0] eq "--genmenu") {
	my $h = hashify($config_in);
	
	print "Dynamic {\n";
	gen_menu($h, "");
	print "}\n";
} elsif ($ARGV[0] && $ARGV[1] && $ARGV[0] eq "--update") {
	edit_var($ARGV[1]);
} else {
	exit 0;
}

# edit_var(arg)
sub edit_var {
	my $arg = shift;
	my @sec = split(/\//, $arg);
	my $hashref = hashify($config_in);
	my $h = $hashref;
	my $did = 0;
	
	# Traverse to the needed section.
	foreach my $i (0..$#sec - 1) {
		$h = $h->{$sec[$i]};
	}
	
	# Read through the variables file.
	open(INFILE, "<".$vars_path) or die "Could not find: ".$vars_path."\n";
	while (<INFILE>) {
		# int
		if ($_ =~ /int\|$arg\|(.+)\|(\d+)\|(\d+)/) {
			edit_int($sec[$#sec], $h, $1, $2, $3);
			$did = 1;
		# file
		} elsif ($_ =~ /file\|$arg\|(.+)/) {
			edit_file($sec[$#sec], $h, $1);
			$did = 1;
		# dir
		} elsif ($_ =~ /dir\|$arg\|(.+)/) {
			edit_dir($sec[$#sec], $h, $1);
			$did = 1;
		# bool
		} elsif ($_ =~ /bool\|$arg\|(.+)/) {
			edit_bool($sec[$#sec], $h, $1);
			$did = 1;
		# option
		} elsif ($_ =~ /option\|$arg\|(.+)\|(.+)\|(\d)/) {
			edit_option($sec[$#sec], $h, $1, $2, $3);
			$did = 1;
		# string
		} elsif ($_ =~ /string\|$arg\|(.+)/) {
			edit_string($sec[$#sec], $h, $1);
			$did = 1;
		}
	}
	close(INFILE);
	
	# If this variable does not have a vars entry, we will treat it
	# as a string.
	if (!$did) {
		edit_string($sec[$#sec], $h, "No description available.");
	}
	
	# Write updated hash to file.
	open my $fh, '>', $config_out or die "Could not open config for writing!\n";
	write_hash($hashref, $fh, "");
	close $fh;
	
	# Reload pekwm
	my $reload = `kill -HUP \$(xprop -root _NET_WM_PID | awk '/_NET_WM_PID/ { print \$3 }')`;
}

# edit_int(key, value, desc, min, max)
sub edit_int {
	my ($key, $hashref, $desc, $min, $max) = @_;
	my $nval;
	my $value = $hashref->{$key};
	
	# Check that we have the correct data type.  If not, set the start
	# value to the min value.
	if (!($value =~ /\d+/)) {
		$value = $min;
	}
	
	$nval = `zenity --title=\"$key\" --scale --text=\"$desc\" --value=$value --min-value=$min --max-value=$max --step=1`;
	chomp($nval);
	
	# If cancel wasn't pressed...
	if ($nval) {
		# Set to the new value.
		$hashref->{$key} = $nval;
	} else {
		exit 0;
	}
}

# edit_string(key, value, desc)
sub edit_string {
	my ($key, $hashref, $desc) = @_;
	
	my $nval = `zenity --title=\"$key\" --entry --text=\"$desc\" --entry-text=\"$hashref->{$key}\"`;
	chomp($nval);
	
	if ($nval) {
		# Set to the new value.
		$hashref->{$key} = $nval;
	} else {
		exit 0;
	}
}

# edit_file(key, value, desc)
sub edit_file {
	my ($key, $hashref, $desc) = @_;
	
	my $nval = `zenity --title=\"$key\" --file-selection --text=\"$desc\" --filename=\"$hashref->{$key}\"`;
	chomp($nval);
	
	if ($nval) {
		# Set to the new value.
		$hashref->{$key} = $nval;
	} else {
		exit 0;
	}
}

# edit_dir(key, value, desc)
sub edit_dir {
	my ($key, $hashref, $desc) = @_;
	
	my $nval = `zenity --title=\"$key\" --file-selection --directory --text=\"$desc\" --filename=\"$hashref->{$key}\"`;
	chomp($nval);
	
	if ($nval) {
		# Set to the new value.
		$hashref->{$key} = $nval;
	} else {
		exit 0;
	}
}

# edit_bool(key, value, desc)
sub edit_bool {
	my ($key, $hashref, $desc) = @_;
	my ($TRUE, $FALSE);
	
	if ($hashref->{$key} eq "True") {
		$TRUE = "TRUE";
		$FALSE = "FALSE";
	} else {
		$TRUE = "FALSE";
		$FALSE = "TRUE";
	}
	
	my $nval = `zenity --title=\"$key\" --list --radiolist --text=\"$desc\" --column \"Check\" --column \"Option\" $TRUE True $FALSE False`;
	chomp($nval);
	
	if ($nval) {
		# Set to the new value.
		$hashref->{$key} = $nval;
	} else {
		exit 0;
	}
}

# edit_bool(key, value, desc)
sub edit_option {
	my ($key, $hashref, $desc, $opt, $multi) = @_;
	my ($nval, $selstr, $tmp, $gotone, @multicur);
	my @opts = split(/:/, $opt);
	
	if (!$multi) {
		$selstr = "";
		
		foreach (@opts) {
			if ($_ eq $hashref->{$key}) {
				$selstr .= "TRUE ".$_." ";
			} else {
				$selstr .= "FALSE ".$_." ";
			}
		}
		
		$nval = `zenity --title=\"$key\" --list --radiolist --text=\"$desc\" --column \"Check\" --column \"Option\" $selstr`;
	} else {
		$selstr = "";
		@multicur = split(/ /, $hashref->{$key});
		
		foreach my $tmp (@opts) {
			$gotone = 0;
			
			foreach (@multicur) {
				if ($tmp eq $_) {
					$selstr .= "TRUE ".$_." ";
					$gotone = 1;
				}
			}
			
			if (!$gotone) {
				$selstr .= "FALSE ".$tmp." "
			}
		}
		
		$nval = `zenity --title=\"$key\" --list --checklist --separator=\" \" --text=\"$desc\" --column \"Check\" --column \"Option\" $selstr`;
	}
	chomp($nval);
	
	if ($nval) {
		# Set to the new value.
		$hashref->{$key} = $nval;
	} else {
		exit 0;
	}
}

# write_hash(hash ref)
# Write out the hash to config file format.
sub write_hash {
	my ($hashref, $fh, $tabs) = @_;
	
	foreach my $key (sort(keys %$hashref)) {
		if (ref($hashref->{$key}) eq 'HASH') {
			print $fh $tabs.$key." {\n";
			write_hash($hashref->{$key}, $fh, $tabs."\t");
			print $fh $tabs."}\n";
		} else {
			print $fh $tabs.$key." = \"".$hashref->{$key}."\"\n";
		}
	}
}

# gen_menu(hash ref, location string)
# This will generate the menu.
sub gen_menu {
	my ($hashref, $loc) = @_;
	
	foreach my $key (sort(keys %$hashref)) {
		if (ref($hashref->{$key}) eq 'HASH') {
			print "Submenu = \" ".$key."\" {\n";
			gen_menu($hashref->{$key}, $loc.$key."/");
			print "}\n";
		} else {
			gen_entry($key, $hashref->{$key}, $loc);
		}
	}
}

# gen_entry(key, value, location)
sub gen_entry {
	my ($key, $value, $loc) = @_;
	
	# Boolean value
	if ($value eq "True") {
		print "Entry = \" ".$key."\" { Icon = \"check.png\"; Actions = \"Exec ".$path." --update ".$loc.$key."\" }\n";
	} elsif ($value eq "False") {
		print "Entry = \" ".$key."\" { Icon = \"uncheck.png\"; Actions = \"Exec ".$path." --update ".$loc.$key."\" }\n";
	} else {
		print "Entry = \" ".$key." [".$value."]\" { Actions = \"Exec ".$path." --update ".$loc.$key."\" }\n";
	}
}

# hashify(file name)
# This should read a valid config file and store all of it into
# a logical hash table.
sub hashify {
	# Filename parameter
	my $file = shift;

	# Replace ~ with home directory
	$file =~ s/^~/$ENV{'HOME'}/;

	my $comment = 0;
	my ($key, $value, $hval, $val);
	my @name_stack;
	my @hash_stack;
	
	push(@hash_stack, {});
	
	# Open file for reading.
	open(INFILE, $file) or return pop(@hash_stack);

	# Read through file, line by line.
	while (my $line = <INFILE>) {
		# Remove the new line character.
		chomp($line);
		
		# If we are currently within a comment, we need to check for
		# the end comment symbols.
		if ($comment) {
			# If we hit the end comment characters, we are done
			# commenting.
			if ($line =~ s<.*\*/><>) { $comment = 0; }
			# We are still commenting, so just skip the rest of this
			# code.
			else { next; }
		}
		
		# Remove /* bleh */ one liners.
		$line =~ s</\*.*\*/><>;
	
		# Remove /* bleh.  This also means we have begun a multi-line
		# comment, so set $comment to 1.
		if ($line =~ s</\*.*><>) { $comment = 1; next; }
		
		# If we're not currently in a multi-line comment,
		# remove // style comments.
		if (!$comment) { $line =~ s<//.*><>; }
		
		# If we have no line or this is a comment, skip the rest.
		if (!$line || $comment) { next; }
		
		# Is this a block?
		while ($line =~ s/(\w+)\s*\{// ||
				$line =~ s/\"(.+)\"\s*\{//) {
			push(@name_stack, $1);
			push(@hash_stack, {});
			
			# Is this a variable?
			# This, by the way, is needed for those one lined nested
			# blocks and variables.
			while ($line =~ s/^\s*(\w+)\s*=\s*\"(.*)\"// ||
					$line =~ s/^\s*\"([\w\s]+)\"\s*=\s*\"(.*)\"//) {
				$hash_stack[$#hash_stack]->{$1} = $2;
				
				# Check if we have a bracket coming up, signifying
				# the end of a block.
				while ($line =~ s/^\s*\}//) {
					$val = pop(@name_stack);
					$hval = pop(@hash_stack);
					
					# Add each key/value pair to the section/subsection.
					while (($key, $value) = each(%$hval)) {
						$hash_stack[$#hash_stack]->{$val}->{$key} = $value;
					}
				}
			}
		}
		
		# Is this a variable?
		if (	$line =~ s/^\s*(\w+)\s*=\s*\"(.*)\"// ||
				$line =~ s/^\s*\"([\w\s]+)\"\s*=\s*\"(.*)\"//) {
			$hash_stack[$#hash_stack]->{$1} = $2;
			
			# Check if we have a bracket coming up, signifying
			# the end of a block.
			while ($line =~ s/^\s*\}//) {
				$val = pop(@name_stack);
				$hval = pop(@hash_stack);
				
				# Add each key/value pair to the section/subsection.
				while (($key, $value) = each(%$hval)) {
					$hash_stack[$#hash_stack]->{$val}->{$key} = $value;
				}
			}
		}
		
		# End of block?
		while ($line =~ s/^\s*\}//) {
			$val = pop(@name_stack);
			$hval = pop(@hash_stack);
			
			# Add each key/value pair to the section/subsection.
			while (($key, $value) = each(%$hval)) {
				$hash_stack[$#hash_stack]->{$val}->{$key} = $value;
			}
		}
	}
	
	# Return fully assembled hash.
	return pop(@hash_stack);
}
