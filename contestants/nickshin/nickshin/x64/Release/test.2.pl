#!/usr/bin/perl -w
#
# NICK SHIN
# nick.shin@gmail.com
# Written: Dec 2014
# License: public domain
#
# For: http://churchillnavigation.com/challenge/


# this file helped me process the summary report (via test.1.sh) - which is
# still a little terse.  so, this script will extract the interested data,
# run calculations and then dump the results.


use strict;
use warnings;

sub parse_file
{
	my $fname = 'zzz_report.txt';
	open FH, "< $fname" || die "unable to open $fname : $!";
    my @lines = <FH>;
    close FH;

	my ( $level, $seed, $time ) = ( -1, 0, -1 );
	my @reference = ();		# reference.dll time queries
	my @nickshin = ();		# nickshin.dll time queries
	my @ram = ();			# nickshin.dll mem usage
	my $memused = 0;		# mem usage helper
	foreach ( @lines ) {
		if ( /memory used: (\d+)MB/ ) { $memused = $1; } # nickshin mem usage
		next unless ( m!logs.(\d+)/log\.(.+)\.txt:.+ avg (\d+\.\d+)ms/! );
		my ( $l, $s, $t ) = ( $1, $2, $3 );
		if ( $level != $l ) { # new level
			if ( $l ) # do this only for level > 0
				{ print_summary( $level, \@nickshin, \@reference, \@ram ); }
			# reset
			@nickshin = ();
			@reference = ();
			@ram = ();
			$level = $l;
			$seed = $s;
			push @nickshin, $t;
			next;
		}
		if ( $seed eq $s ) {
			push @reference, $t;
			push @ram, $memused; # record nickshin mem usage here...
		} else {
			$seed = $s;
			push @nickshin, $t;
		}
    }
	print_summary( $level, \@nickshin, \@reference, \@ram );
}

sub print_summary
{
	my ( $level, $nnickshin, $rreference, $rram ) = @_;

	# the 4-ways to count the size of an array reference
	my $count = @$nnickshin;
#	my $snick = @{$nnickshin};
#	my $sref = scalar( @{$rreference} );
#	my $sram = $#{$rram} + 1;

#	print "$level - $count $snick $sref $sram\n";

	# ------------------------------------------------------------
	# dump summary
	print "lev.$level";
	my $total = 0;
	foreach( @{$nnickshin} ) { $total += $_; }
	my $ave = sprintf( "%.4f", $total / $count );
	print "\tnickshin:  ave:$ave"."ms/query - ";
	
	my $mem = 0;
	foreach( @{$rram} ) {
		if ( $_ > $mem ) { $mem = $_; }
	}
	my $boom = ( $mem > 512 ) ? ' ***' : '';
	print "ram:$mem"."MB$boom\n";

	$total = 0;
	foreach( @{$rreference} ) { $total += $_; }
	$ave = sprintf( "%.04f", $total / $count );
	print "\treference: ave:$ave"."ms/query\n";

	# ------------------------------------------------------------
	# best and worst performance runs
	my $bestnick = 0;
	my $bestref = 0;
	my $bestcalc = 0;
	my $worstnick = 0;
	my $worstref = 0;
	my $worstcalc = 0;
	my $calc;
	for ( my $i = 0; $i < $count; $i++ ) {
		# the 2-ways to dereference an array
		my $nick = $nnickshin->[$i];
		my $ref  = $$rreference[$i];

		if ( $nick < $ref ) { # faster
			$calc = sprintf( "%.2f", $ref / $nick );
			if ( $calc > $bestcalc ) {
				$bestnick = $nick;
				$bestref = $ref;
				$bestcalc = $calc;
			} else {
				if ( ! $worstcalc ) { $worstcalc = 100000; }
				if ( $calc < $worstcalc ) {
					$worstnick = $nick;
					$worstref = $ref;
					$worstcalc = $calc;
				}
			}
		} else { # slower - to be denoted as (-)
			$calc = sprintf( "%.2f", -1 * ( $nick / $ref ) );
			if ( ! $bestcalc ) { $bestcalc = -100000; }
			if ( $calc > $bestcalc ) {
				$bestnick = $nick;
				$bestref = $ref;
				$bestcalc = $calc;
			}
			elsif ( $calc < $worstcalc ) {
				$worstnick = $nick;
				$worstref = $ref;
				$worstcalc = $calc;
			}
		}
	}
	print "\t----- nickshin           reference\n";
	$bestcalc .= ( $bestcalc > 0 ) ? "x  faster" : "x  slower";
	$worstcalc .= ( $worstcalc > 0 ) ? "x  faster" : "x  slower";
	print "\tbest  $bestnick"."ms/query vs. $bestref"."ms/query :: $bestcalc\n";
	print "\tworst $worstnick"."ms/query vs. $worstref"."ms/query :: $worstcalc\n";
}

parse_file();


