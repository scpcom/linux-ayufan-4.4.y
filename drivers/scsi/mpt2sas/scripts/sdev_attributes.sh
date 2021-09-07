#!/bin/bash

cd /sys/class/scsi_device
for i in *; do
	echo -e \\n
	echo $i
	cd $i/device
	for f in *; do
		if [ -f $f -a -r $f ]; then
			echo -n -e \\t "$f:";
			cat $f ;
		fi;
	done;
	cd ../..
done;
echo -e \\n
