#!/bin/bash

if (( $# != 1 )); then
	echo -e \\n"useage: set_device_timeout <level>"
	echo -e \\n
	exit 1
fi

cd /sys/class/scsi_device
for i in *; do
	cd $i/device
	if [ `cat type` == "0" ]; then
		echo $1 > timeout
		echo [$i] timeout=`cat timeout`
	fi;
	cd -
done;
