#!/bin/bash
# IPSEC Module build script
set -e

PKG="openswan-2.6.41"

# Optionaly download the sources from web
if [[ ! -f "${PKG}.tar.gz" ]]; then
	wget --no-check-certificate http://download.openswan.org/openswan/${PKG}.tar.gz
fi

# Extract sources localy
tar -zxvf ${PKG}.tar.gz
cd ${PKG}

# Apply marvell patch over openswan sources
patch -p1 < ../mv_openswan_2_6_41.patch

# Build ipsec module
make KERNELSRC=../../../ module ARCH=arm

# Copy to modules output directory
if [ "$1" != "" ]; then
	mkdir -p ${1}/kernel/ipsec
	cp modobj26/ipsec.ko ${1}/kernel/ipsec
fi
