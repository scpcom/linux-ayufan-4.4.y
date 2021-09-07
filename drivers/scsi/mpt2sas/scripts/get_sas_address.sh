#!/bin/bash
if (( $# != 1 ))
then
	clear
	echo Usage: ./get_sas_address /dev/sg
	echo -e This is not supported for direct attached SATA drives.\\n
	exit 1
fi;
sg_vpd -p di_port -q $1
