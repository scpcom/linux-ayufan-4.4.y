/*
 * arch/arm/plat-orion/include/plat/mv_memcpy.h
 *
 * Marvell memcpy platform device data definition file.
 */

#ifndef __PLAT_MV_MEMCPY_H
#define __PLAT_MV_MEMCPY_H

#include <linux/dmaengine.h>
#include <linux/mbus.h>

#define MV_MEMCPY_NAME		"mv_memcpy"

struct mbus_dram_target_info;

struct mv_memcpy_platform_data {
	struct mbus_dram_target_info	*dram;
	size_t				pool_size;
	unsigned int                    coalescing;
};

#endif
