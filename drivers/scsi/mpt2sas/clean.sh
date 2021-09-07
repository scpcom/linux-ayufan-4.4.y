#!/bin/bash
#
# clean.sh : a script for cleaning the driver source tree.
#

rm -f .*.o.*
rm -f .*.ko.*
rm -f *.ko
rm -f *.o
rm -f *~
#rm -f csmi/*~
rm -f *.mod.*
rm -fr .tmp_versions
rm -fr Modules.symvers
rm -fr Module.symvers
rm -f output.log
rm tags
dmesg -c > /dev/null

exit 0
