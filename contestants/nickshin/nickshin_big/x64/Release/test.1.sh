#!/bin/bash
#
# NICK SHIN
# nick.shin@gmail.com
# Written: Dec 2014
# License: public domain
#
# For: http://churchillnavigation.com/challenge/


# this file helped me automate a bunch of tests.
#
# basically, all of the log files from each tests are saved for analysis later.
# then, the report parses the log files and generate a summary for me to help
# make a decision on what quadtree level i should use...


# ============================================================
# generators

generate_seeds()
{
	echo "=== generating seeds"
	for x in {0..200}; do
		./point_search.exe challenge.x64.v1.dll | grep Random | awk '{ print $4 }' | tee -a zzz_seeds.txt
	done
}

generate_images()
{
	echo "=== generating images"
	if [ ! -d 'dump' ]; then
		mkdir dump
	fi
	while read line; do
		echo "$line"
		./point_search.exe challenge.x64.v4.0.0.dump3.dll
		mv dump.3.bmp dump/dump.${line}.bmp
	done < zzz_seeds.txt
}

generate_webpage()
{
	echo "=== generating webpage"
	printf "*** debug\n\n" > zzz_bmp.txt
	printf "*** MID\n\n" > zzz_bmp.txt
	printf "*** NORTH\n\n" >> zzz_bmp.txt
	printf "*** WEST\n\n" >> zzz_bmp.txt
	printf "*** SOUTH\n\n" >> zzz_bmp.txt
	printf "*** EAST\n\n" >> zzz_bmp.txt
	printf "*** NORTHWEST\n\n" >> zzz_bmp.txt
	printf "*** NORTHEAST\n\n" >> zzz_bmp.txt
	printf "*** SOUTHWEST\n\n" >> zzz_bmp.txt
	printf "*** SOUTHEAST\n\n" >> zzz_bmp.txt
	echo '' > zzz_bmp.html
	while read line; do
		echo "<img src='dump/dump.${line}.bmp'><br>$line<p>" >> zzz_bmp.html
		echo $line >> zzz_bmp.txt
	done < zzz_bmp_seeds.txt
}


# ============================================================
# METRICS

metrics()
{
	echo "=== TEST RUN"
	for x in {0..12}; do
		if [ ! -d "logs.$x" ]; then
			mkdir logs.$x
		fi
	done
	while read SEED; do
		for x in {0..12}; do
			# COMPARE THE TWO DLLs !!!  ^_^
			./point_search.exe challenge.x64.v4.1.0.dll reference.dll -s$SEED -t$x

			# SAVE THE LOG FILE FOR ANALYSIS LATER...
			mv log.txt logs.$x/log.${SEED}.txt
		done
	done < zzz_seeds.txt
}

report()
{
	echo "=== generating report"

	echo "!!! CRASH:" > zzz_report.txt
		# SCANNING FOR ANYTHING THAT DOES NOT END WITH "done" (i.e. "crashed")
		egrep "Making queries" -H logs.*/* | egrep -v "done" >> zzz_report.txt

	echo "!!! DIFF:" >> zzz_report.txt
		# SCANNING FOR ANYTHING THAT DOES NOT END WITH "match" (i.e. "differ")
		egrep and -H logs.*/* | grep reference.dll | egrep -v match >> zzz_report.txt

	echo "!!! SUMMARY:" >> zzz_report.txt
		for x in {0..12}; do
			egrep "Making queries" -A1 -H logs.$x/* >> zzz_report.txt
		done
}


# ============================================================
# MAIN

generate_seeds
generate_images
generate_webpage

metrics
report

