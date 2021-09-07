#!/bin/bash

cd /sys/class/scsi_host
for i in *; do
	echo -e \\n
	echo $i
	cd $i
	for f in *; do
		if [ -f $f -a -r $f ]; then
			echo -n -e \\t "$f:";
			cat $f ;
		fi;
	done;
	cd ..
done;
