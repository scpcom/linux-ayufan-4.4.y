#!/bin/bash
#
# load.sh : a helper script for loading the drivers
#

# toggling bits for enabling scsi-mid layer logging

# enable sense data
#sysctl -w dev.scsi.logging_level=0x1000

# enable scanning debuging
sysctl -w dev.scsi.logging_level=0x1C0

# enable sense data and scanning
#sysctl -w dev.scsi.logging_level=0x11C0

# loading scsi mid layer and transports
modprobe sg
modprobe sd_mod
modprobe scsi_transport_sas
modprobe raid_class

# loading the driver
insmod mpt2sas.ko
#insmod mpt2sas.ko logging_level=0x310
#insmod mpt2sas.ko max_queue_depth=128 max_sgl_entries=32 logging_level=0x620
#insmod mpt2sas.ko max_queue_depth=500 max_sgl_entries=32 logging_level=0x620

# fw events + tm + reset + reply
#insmod mpt2sas.ko logging_level=0x2308

# work task + reply
#insmod mpt2sas.ko logging_level=0x210
#insmod mpt2sas.ko logging_level=0x200

# fw events + reply
#insmod mpt2sas.ko logging_level=0x208

# fw events + work task + reply
#insmod mpt2sas.ko logging_level=0x218

# handshake + init
#insmod mpt2sas.ko logging_level=0x420

# everything
#insmod mpt2sas.ko logging_level=0xFFFFFFF

# reset
#insmod mpt2sas.ko logging_level=0x2000

#ioctls
#insmod mpt2sas.ko logging_level=0x8000

#init
#insmod mpt2sas.ko logging_level=0x20

#config
#insmod mpt2sas.ko logging_level=0x800

exit 0