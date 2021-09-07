#!/bin/bash
#set -x
if (( $# != 1 )); then
	echo -e \\n"useage: set_logging_level <level>"
	echo -e \\n
	exit 1
fi

scsi_host="/sys/class/scsi_host"
cd ${scsi_host}
subfolders=`ls -1`
for i in ${subfolders};  do
	cd ${i}
	if [ `cat proc_name` != "mpt2sas" ]; then
		cd ${scsi_host}
		continue;
	fi;
	echo $1 > logging_level
	logging_level=`cat logging_level`
	echo for ${i} logging_level=$logging_level
	cd ${scsi_host}
done;
