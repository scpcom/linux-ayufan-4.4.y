#!/bin/sh
. mv_conf.mk
subtxt(){

    if [ ! -f "$1" ] || [ ! -w "$1" ]; then
	echo "File does not exist or is not writable."
	exit 1
    fi

    if [ "$2" == "a" ];then
        if [ "$SUPPORT_THOR" == "y"  ];then

            grep SCSI_MV_61xx "$1" >/dev/null 2>&1
	    if [ "$?" == "0" ];then
	        cat "$1"
                return
	    fi

            sed -e '/if SCSI_LOWLEVEL && SCSI/{
                    a\
config SCSI_MV_61xx\
	tristate "Marvell Storage Controller 6121/6122/6141/6145"\
	depends on SCSI && BLK_DEV_SD\
	help\
		Provides support for Marvell 61xx Storage Controller series.\n
}' "$1"

        elif [ "$SUPPORT_VANIR" == "y" ]; then

            grep SCSI_MV_94xx "$1" >/dev/null 2>&1
            if [ "$?" == "0" ];then
            cat "$1"
               return
            fi

            sed -e '/if SCSI_LOWLEVEL && SCSI/{
                    a\
config SCSI_MV_94xx\
	tristate "Marvell Storage Controller 9180/9480"\
	depends on SCSI && BLK_DEV_SD\
	help\
		Provides support for Marvell 94xx Storage Controller series.\n
}' "$1"

        elif [ "$SUPPORT_ODIN" == "y" ]; then
            grep SCSI_MV_64xx "$1" >/dev/null 2>&1
            if [ "$?" == "0" ];then
            cat "$1"
               return
            fi

            sed -e '/if SCSI_LOWLEVEL && SCSI/{
                    a\
config SCSI_MV_64xx\
	tristate "Marvell Storage Controller 6430/6320/6440/6445/6480/6485"\
	depends on SCSI && BLK_DEV_SD\
	help\
		Provides support for Marvell 64xx Storage Controller series.\n
}' "$1"
        else
	   echo "Cannot find the specified product, define mv_conf.mk."
	   exit 1
        fi
    else
        if [ "$SUPPORT_THOR" == "y"  ];then
            sed -e '/SCSI_MV_61xx/,+5 d' "$1"
        elif [ "$SUPPORT_VANIR" == "y"  ];then
            sed -e '/SCSI_MV_94xx/,+5 d' "$1"
        elif [ "$SUPPORT_ODIN" == "y"  ];then
            sed -e '/SCSI_MV_64xx/,+5 d' "$1"
        else
	   echo "Cannot find the specified product, define mv_conf.mk."
	   exit 1
        fi
    fi
}

# $1 is supposed to be the $KERNEL_SRC/drivers/scsi
if [ ! -d "$1" ];then
    echo "Cannot find the specified directory."
    exit 1
fi

cd "$1"
subtxt Kconfig $2 > Kconfig.new
mv Kconfig Kconfig.orig
mv Kconfig.new Kconfig
