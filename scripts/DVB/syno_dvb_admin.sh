ACTION="$1"

DEVICE_TABLE_FOLDER="/lib/udev/devicetable/backports_dvb"
DVB_HANDLER_FOLDER="/lib/udev/script/backports_dvb"
DVB_HANDLER="usb-dvb-util-backports.sh"


if [ "${ACTION}" = "start" ]; then
	echo "yes" > ${DVB_HANDLER_FOLDER}/DTV_enabled
	[ -e "${DVB_HANDLER_FOLDER}/manual_gen_hotplug_backports.sh" ] && ${DVB_HANDLER_FOLDER}/manual_gen_hotplug_backports.sh "add"
else
	echo "no" > ${DVB_HANDLER_FOLDER}/DTV_enabled
	[ -e "${DVB_HANDLER_FOLDER}/manual_gen_hotplug_backports.sh" ] && ${DVB_HANDLER_FOLDER}/manual_gen_hotplug_backports.sh "remove"
fi
