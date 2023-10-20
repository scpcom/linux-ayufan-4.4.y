#!/bin/sh

export PATH="${PATH}:/opt/OpenWrt-Toolchain-comcerto2000-for-arm_v7-a-gcc-4.5-linaro_glibc-2.14.1_eabi/bin"

CROSS="/opt/OpenWrt-Toolchain-comcerto2000-for-arm_v7-a-gcc-4.5-linaro_glibc-2.14.1_eabi/bin/arm-openwrt-linux-"
STAGING_DIR="staging_dir"
KERNEL_CONFIG="STG-540_Kernel.config"

echo "creating ${STAGING_DIR} ..."
mkdir ${STAGING_DIR}
export STAGING_DIR="${STAGING_DIR}"

cp -av ${KERNEL_CONFIG} .config

#make HOSTCFLAGS="-O2 -I/home/andy/mindspeed/20140325/sdk-comcerto-openwrt-c2k_nas_3.1-rc2/staging_dir/host/include -Wall -Wmissing-prototypes -Wstrict-prototypes" ARCH=arm CC="arm-openwrt-linux-gnueabi-gcc" CFLAGS="-O3 -pipe -march=armv7-a -mtune=cortex-a9 -fno-caller-saves -mthumb -fhonour-copts  -msoft-float" CROSS_COMPILE=arm-openwrt-linux-gnueabi- KBUILD_HAVE_NLS=no CONFIG_SHELL=/bin/bash oldconfig
make V=99 HOSTCFLAGS="-O2 -Wall -Wmissing-prototypes -Wstrict-prototypes" ARCH=arm CC="${CROSS}gcc" CFLAGS="-O3 -pipe -march=armv7-a -mtune=cortex-a9 -fno-caller-saves -mthumb -fhonour-copts  -msoft-float" CROSS_COMPILE=${CROSS} KBUILD_HAVE_NLS=no uImage
