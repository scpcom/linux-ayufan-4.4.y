/*
 * xhci-comcerto.h - Comcerto-2000 Platform specific routienes.
 *
 * Author: Makarand Pawagi
 */


#ifndef __XHCI_COMCERTO_2000_H__
#define __XHCI_COMCERTO_2000_H__



/* Exported Functions */


extern void comcerto_start_xhci(void);
extern void comcerto_stop_xhci(void);

extern int comcerto_xhci_bus_resume(struct usb_hcd *hcd);
extern int comcerto_xhci_bus_suspend(struct usb_hcd *hcd);



#endif /* __XHCI_COMCERTO_2000_H__ */
