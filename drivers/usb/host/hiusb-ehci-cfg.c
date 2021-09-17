#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/io.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/hardware.h>

extern long long get_chipid(void);
extern void hiusb_start_hcd_hi3536(void);
extern void hiusb_stop_hcd_hi3536(void);

void hiusb_start_hcd(void)
{
	long long chipid = get_chipid();
	switch (chipid) {
	default:
		hiusb_start_hcd_hi3536();
		break;
	}
}
EXPORT_SYMBOL(hiusb_start_hcd);

void hiusb_stop_hcd(void)
{
	long long chipid = get_chipid();
	switch (chipid) {
	default:
		hiusb_stop_hcd_hi3536();
		break;
	}
}
EXPORT_SYMBOL(hiusb_stop_hcd);
