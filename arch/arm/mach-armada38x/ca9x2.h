#ifndef __MACH_MV_CA9X2_H
#define __MACH_MV_CA9X2_H

/*
 * Physical base addresses
 */
#define MV_CA9X2_L2CC_OFFSET	(0x8000)

#define MV_CA9X2_MPIC		(0xC000)
#define A9_MPCORE_SCU		(MV_CA9X2_MPIC + 0x0000)
#define A9_MPCORE_GIC_CPU	(MV_CA9X2_MPIC + 0x0100)
#define A9_MPCORE_GIT		(MV_CA9X2_MPIC + 0x0200)
#define A9_MPCORE_TWD		(MV_CA9X2_MPIC + 0x0600)
#define A9_MPCORE_GIC_DIST	(MV_CA9X2_MPIC + 0x1000)

#endif /* __MACH_MV_CA9X2_H */
