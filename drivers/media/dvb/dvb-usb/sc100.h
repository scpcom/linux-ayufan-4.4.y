#ifndef _SC100_H_
#define _SC100_H_

#define DVB_USB_LOG_PREFIX "sc100"
#include "dvb-usb.h"

#define deb_xfer(args...) dprintk(dvb_usb_sc100_debug, 0x02, args)
#define deb_rc(args...)   dprintk(dvb_usb_sc100_debug, 0x04, args)
#endif
