/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************/

/* Marvell Telephony Adaptation Layer */

#ifndef _TAL_H_
#define _TAL_H_

#include "mvOs.h" /* for kernel abstraction wrappers */

/* Defines */
#define TAL_MAX_PHONE_LINES	32

/* Enumerators */
typedef enum {
	TAL_PCM_FORMAT_1BYTE = 1,
	TAL_PCM_FORMAT_2BYTES = 2,
	TAL_PCM_FORMAT_4BYTES = 4
} tal_pcm_format_t;

typedef enum {
	TAL_STAT_OK = 0,
	TAL_STAT_BAD_PARAM,
	TAL_STAT_INIT_ERROR
} tal_stat_t;

/* Structures */
typedef struct {
	tal_pcm_format_t pcm_format;
	unsigned short pcm_slot[TAL_MAX_PHONE_LINES];
	unsigned char sampling_period;
	unsigned short total_lines;
	unsigned short test_enable;
} tal_params_t;

typedef struct {
	int tdm_init;
	unsigned int rx_miss;
	unsigned int tx_miss;
	unsigned int rx_over;
	unsigned int tx_under;
} tal_stats_t;

typedef struct {
	void (*tal_mmp_rx_callback)(unsigned char* rx_buff, int size);
	void (*tal_mmp_tx_callback)(unsigned char* tx_buff, int size);
} tal_mmp_ops_t;

/* APIs */
tal_stat_t tal_init(tal_params_t* tal_params, tal_mmp_ops_t* mmp_ops);
tal_stat_t tal_stats_get(tal_stats_t* tal_stats);
tal_stat_t tal_pcm_start(void);
tal_stat_t tal_pcm_stop(void);
tal_stat_t tal_exit(void);

#endif /* _TAL_H */
