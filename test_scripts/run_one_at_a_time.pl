#!/bin/perl

$n = $#ARGV+1;

for($i=0; $i<$n; $i++) {
	$cmd = "point_search.exe $ARGV[$i] -s75BCD15-159A55E5-1F123BB5-5491333-34DAD465";
	$out = `$cmd`;
	print($out);
}
