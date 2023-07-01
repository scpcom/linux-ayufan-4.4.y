#ifndef _SC100_H_
#define _SC100_H_

#define DVB_USB_LOG_PREFIX "sc100"
#include "dvb-usb.h"

#define deb_info(args...)   dprintk(dvb_usb_sc100_debug, 0x01, args)
#endif
