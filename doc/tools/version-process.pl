#!/usr/bin/env perl
#
# Copyright © 2009 the pekwm development team
#

use strict;
use warnings;

use POSIX qw(strftime);
my $infile = $ARGV[0];
my $outfile = $ARGV[1];
my $dbv = $ARGV[2];

#-- set up date variables
my @lt        = localtime();
my $d_compact = strftime('%Y%m%d', @lt);
my $d_full    = strftime('%B %d, %Y', @lt);

#-- version variable
open(VER, "tools/version");
my $version = <VER>;
chomp($version);
close(VER);

#-- actual out strings
my $o_cv = sprintf("<!ENTITY current-version \"%s\">\n", $version);
my $o_dc = sprintf("<!ENTITY dstring-compact \"%s\">\n", $d_compact);
my $o_df = sprintf("<!ENTITY dstring-full \"%s\">\n", $d_full);

my $h_desc = sprintf("Documentation corresponding to pekwm version %s, last updated %s.\n",
                     $version, $d_compact);

open(INPUT, "<$infile") or die $!;
my @invar  = <INPUT>;
close(INPUT);
my @outvar = ();
my $inloop=0;
foreach (@invar) {
  if(m/BEGIN HEADER/g) {
    $inloop=1;
    push(@outvar, "\"$dbv\" [\n");
  } elsif(m/END HEADER/g) {
    $inloop=0;
    push(@outvar, $_);
  } elsif(m/BEGIN VERSIONING/g) {
    $inloop=1;
    push(@outvar, $_, $o_cv, $o_dc, $o_df)
  } elsif(m/BEGIN HTML VERSIONING/g) {
    $inloop=1;
    push(@outvar, $_, $h_desc);
  } elsif(m/END VERSIONING/g) {
    $inloop=0;
    push(@outvar, $_);
  } elsif($inloop == 0){
    push(@outvar, $_);
  }
}
open(OUT, ">$outfile");
foreach my $str (@outvar) {
print(OUT $str);
}
close(OUT);


