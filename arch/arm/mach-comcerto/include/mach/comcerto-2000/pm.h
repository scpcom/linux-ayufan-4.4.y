/*
 * pm.h
 *
 * Power managemen driver for Comcerto C2K device - internal header file
 *
 * This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ARCH_ARM_C2K_PM_H__
#define __ARCH_ARM_C2K_PM_H__

/* PMU Interrup masks */
#define GPIO0_IRQ               (1 << 0)
#define GPIO1_IRQ               (1 << 1)
#define GPIO2_IRQ               (1 << 2)
#define GPIO3_IRQ               (1 << 3)

#define GPIO4_IRQ               (1 << 4)
#define GPIO5_IRQ               (1 << 5)
#define GPIO6_IRQ               (1 << 6)
#define GPIO7_IRQ               (1 << 7)

#define TIMER0_IRQ              (1 << 8)
#define TIMER1_IRQ              (1 << 9)
#define TIMER2_IRQ              (1 << 10)
#define TIMER3_IRQ              (1 << 11)

#define ZDS_MSIF_IRQ            (1 << 12)
#define RTC_ALM_IRQ             (1 << 13)
#define RTC_PRI_IRQ             (1 << 14)
#define PCIe0_IRQ               (1 << 15)

#define PCIe1_IRQ               (1 << 16)
#define SATA_IRQ                (1 << 17)
#define SATA_MSI_IRQ            (1 << 18)
#define USB2p0_IRQ              (1 << 19)

#define USB3p0_IRQ              (1 << 20)
#define HFE_0_IRQ               (1 << 21)
#define WOL_IRQ                 (1 << 22)
#define CSS_IRQ                 (1 << 23)

#define DUS_DMAC_IRQ            (1 << 24)
#define DUS_UART0_IRQ           (1 << 25)
#define DUS_UART0UARTS2_IRQ     (1 << 26)
#define HFE_1_IRQ               (1 << 27)

#define USB3p0_PM               (1 << 28)
#define PTP0_IRQ                (1 << 29)
#define PTP1_IRQ                (1 << 30)
#define PTP2_IRQ                (1 << 31)

#define JUMP_TO_RESUME_1		        0xe3a00020 	/* mov	r0, #32 */
#define JUMP_TO_RESUME_2		        0xe590f000 	/* ldr	pc, [r0] */

/*
 * Two Bytes are used as shared memory between Host and UtilPE
 * One for installing the Suspend Event and Return Resume location
 * 2nd to pass the bitmask
 */

#define HOST_UTILPE_SHARED_ADDRESS     		COMCERTO_PMU_IRAM_LOC

#define HOST_UTILPE_SHARED_BITMASK      		(HOST_UTILPE_SHARED_ADDRESS+0x4)
#define HOST_UTILPE_SHARED_PFEAXI_FREQ		(HOST_UTILPE_SHARED_ADDRESS + 0x8)
#define HOST_UTILPE_SHARED_TIMER_PERIOD		(HOST_UTILPE_SHARED_ADDRESS + 0xC)
#define HOST_UTILPE_SHARED_TIMER_VALUE		(HOST_UTILPE_SHARED_ADDRESS + 0x10)
#define HOST_UTILPE_SHARED_TIMER_CHAINID		(HOST_UTILPE_SHARED_ADDRESS + 0x14)
#define HOST_UTILPE_SHARED_TIMER_MAGIC		(HOST_UTILPE_SHARED_ADDRESS + 0x18)

/*
 * Types of PMUs that can be accessed directly and require mutual
 * exclusion between profiling tools.
 */
enum arm_pmu_type {
	ARM_PMU_DEVICE_CPU	= 0,
	ARM_NUM_PMU_DEVICES,
};

/* Global Variable for Shared Util-PE interrupt Mask */
extern unsigned host_utilpe_shared_pmu_bitmask;

void c2k_pm_bitmask_store(unsigned int);
unsigned int c2k_pm_bitmask_show(void);

#endif
