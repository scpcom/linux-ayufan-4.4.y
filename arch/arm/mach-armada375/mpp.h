#ifndef __ARCH_DOVE_MPP_H
#define __ARCH_DOVE_MPP_H

enum aurora_mpp_type {
	/*
	 * This MPP is unused.
	 */
	MPP_UNUSED,

	/*
	 * This MPP pin is used as a generic GPIO pin.
	 */
	MPP_GPIO,

	/*
	 * This MPP is used as a SATA activity LED.
	 */
	MPP_SATA_LED,
	/*
	 * This MPP is used as a functional pad.
	 */
	MPP_FUNCTIONAL,

};

struct aurora_mpp_mode {
	int mpp;
	enum aurora_mpp_type type;
};

void aurora_mpp_conf(struct aurora_mpp_mode *mode);

#endif
