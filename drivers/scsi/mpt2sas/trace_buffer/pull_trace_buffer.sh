#!/bin/bash

#set -x
scsi_host="/sys/class/scsi_host"
page_size=4095
offset=0;
total_size=0;
CWD=`pwd`;
file_name='trace_buffer'

cd ${scsi_host};
subfolders=`ls -1`;
for i in ${subfolders};  do
	cd ${i};
	if [ `cat proc_name` != "mpt2sas" ]; then
		cd -;
		continue;
	fi;
	rm -f ${CWD}/${i}.${file_name};
	echo release > host_trace_buffer_enable;
	total_size=`cat host_trace_buffer_size`;
	while [ "${offset}" -lt "${total_size}" ] ; do
		echo $offset > host_trace_buffer;
		cat host_trace_buffer >> ${CWD}/${i}.${file_name};
		let offset=${offset}+${page_size};
	done;
	cd -;
done;
