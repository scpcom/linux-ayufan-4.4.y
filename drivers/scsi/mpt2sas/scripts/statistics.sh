#!/bin/bash

while true ; do
	cd /sys/class/scsi_device
	for i in `ls -1 .`; do
		iodone_cnt=`cat ${i}/device/iodone_cnt`
		iorequest_cnt=`cat ${i}/device/iorequest_cnt`
		state=`cat ${i}/device/state`
		let pending_count=${iorequest_cnt}-${iodone_cnt}
		echo ${i}: pending = ${pending_count}, state = ${state}
	done
	echo -e \\n
	cd /sys/class/scsi_host
	for i in `ls -1 .`; do
		host_busy=`cat ${i}/host_busy`
		echo ${i}: host_busy = ${host_busy}
	done
	sleep 1
	clear
done
