#!/bin/bash
theArmada375Platfrom=`cat /env${BUILD_ARCH}.mak|grep MARVELL_ARMADA375`
theArmadaPlatfrom=`cat /env${BUILD_ARCH}.mak|grep MARVELL_ARMADA`
ARMADA_PLAT_PJ=plat-armada
ARMADA_PLAT=arch/arm/$ARMADA_PLAT_PJ        

BuildArmada375Link()
{
	#check if link to v2
	if [ ! -L "$ARMADA_PLAT" ] ; then
		mv $ARMADA_PLAT ${ARMADA_PLAT}-v1
		ln -s ${ARMADA_PLAT_PJ}-v2 $ARMADA_PLAT
	fi    
}

BuildArmadaLink()
{
	#check if link to v1
	if [ -L "$ARMADA_PLAT" ] ; then
		mv ${ARMADA_PLAT}-v1 $ARMADA_PLAT
	fi    
}

if [ "$theArmada375Platfrom" != "" ] ; then
	echo "ARMADA375 platform found..."
	BuildArmada375Link
else
	if [ "$theArmadaPlatfrom" != "" ] ; then
		echo "Other ARMADA platform found..."
		BuildArmadaLink
	fi
fi
 