#!/bin/bash

if (( $# != 1 )); then
	echo -e \\n"useage: set_device_queue_depth <level>"
	echo -e \\n
	exit 1
fi

cd /sys/class/scsi_device
for i in *; do
	cd $i/device
	if [ `cat type` == "0" ]; then
		echo $1 > queue_depth
		echo [$i] timeout=`cat queue_depth`
	fi;
	cd -
done;
