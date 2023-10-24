/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_QM_H_
#define _MODULE_QM_H_

#include "types.h"
#include "system.h"

// TMU 3 queue assignments
#define	TMU_QUEUE_IPSEC_IN	0
#define	TMU_QUEUE_IPSEC_OUT	1
#define	TMU_QUEUE_RTP_RELAY	2
#define	TMU_QUEUE_REASSEMBLY	3
#define	TMU_QUEUE_PCAP		4
#define	TMU_QUEUE_LRO		5
#define	TMU_QUEUE_RX0_LOW	6
#define	TMU_QUEUE_RX0_HIGH	7
#define	TMU_QUEUE_RX1_LOW	8
#define	TMU_QUEUE_RX1_HIGH	9
#define	TMU_QUEUE_RX2_LOW	10
#define	TMU_QUEUE_RX2_HIGH	11
#define	TMU_QUEUE_WIFI_LOW	12
#define	TMU_QUEUE_WIFI_HIGH	13
#define	TMU_QUEUE_RTP_CUTTHRU	14
#define	TMU_QUEUE_EVENTIND	15

/** Retrieves TMU3 queue
 *
 * @param input_port	original input port
 * @param high_priority high priority flag
 */
static inline u8 get_tmu3_queue(u8 input_port, u32 high_priority)
{
	u8 queue;

	if (input_port == 1)
		queue = TMU_QUEUE_RX1_LOW;
	else if (input_port == 2)
		queue = TMU_QUEUE_RX2_LOW;
#ifdef WIFI_ENABLE
	else if (IS_WIFI_PORT(input_port))
		queue = TMU_QUEUE_WIFI_LOW;
#endif
	else
		queue = TMU_QUEUE_RX0_LOW;

#if defined(COMCERTO_2000_CLASS)
	// For now, if the priority value (AKA mtd->queue) is set to a non-zero value, put
	//	the packet on the high priority TMU queue (which is the low priority queue + 1).
	if (high_priority)
		queue++;
#endif

	return queue;
}

#if defined(COMCERTO_2000_CONTROL) || defined (COMCERTO_2000_TMU)

#define NUM_SHAPERS		10
#define NUM_SCHEDULERS		8
#define NUM_QUEUES		16
#define MAX_SCHEDULER_QUEUES	8

//#define DEFAULT_MAX_QDEPTH 	96

#define QM_ALG_PQ	0
#define QM_ALG_CBWFQ	1
#define QM_ALG_DWRR	2
#define QM_ALG_RR	3
#define QM_ALG_LAST	QM_ALG_RR

#define QM_RX_MTU       1518

#define QM_IFG_SIZE     20
#define QM_CMD_SIZE	128


#define QDEPTH_CONFIG		(1 << 0)
//#define QWEIGHT_CONFIG		(1 << 1)
#define IFG_CONFIG      	(1 << 2)
#define SCHEDULER_CONFIG	(1 << 3)
#define SHAPER_CONFIG 		(1 << 4)

typedef struct tQM_QDesc {
	U8 shaper_num;			// number of shaper assigned to this queue
	U8 sched_num;			// number of scheduler assigned to this queue
} QM_QDesc, *PQM_QDesc;

typedef struct tQM_ShaperDesc {
	U32 baseaddr;
	U32 qmask;			// mask of queues assigned to this shaper
	U32 shaper_rate;
	U32 bucket_size;
} QM_ShaperDesc, *PQM_ShaperDesc;

typedef struct tQM_SchedDesc {
	U32 baseaddr;
	U32 qmask;			// mask of queues assigned to this scheduler
	U32 unused;
	U8 alg;				// current scheduling algorithm
	U8 numqueues;			// number of queues assigned to this scheduler
	U8 current_qnum;		// saved qnum for RR algorithm
	U8 pad[1];
	U32 queue_mask[MAX_SCHEDULER_QUEUES];	// absolute queue number (written as a mask) for each of the scheduler queues
} QM_SchedDesc, *PQM_SchedDesc;

typedef struct tQM_context {
	U8 tmu_id;			// TMU id #
	U8 num_shapers;			// number of active shapers
	U8 num_sched;			// number of schedulers
	U8 chip_revision;		// chip revision level
	QM_ShaperDesc shaper[NUM_SHAPERS];	// shaper descriptors
	QM_SchedDesc sched[NUM_SCHEDULERS];	// scheduler descriptors
	U32 q_len_masks[NUM_QUEUES];
	U8 qresult_regoffset[NUM_QUEUES];
        U16 weight[NUM_QUEUES];
} __attribute__((aligned(32))) QM_context, *PQM_context;

typedef struct tQM_ShaperDesc_ctl {
        U8  enable;                    // enable or disable of a shaper.
	U8  int_wt;
	U16 frac_wt;
	U32 clk_div;
        U32 max_credit;
} QM_ShaperDesc_ctl, *PQM_ShaperDesc_ctl;



typedef struct tQM_context_ctl {
        U8 port;
	U8 tmu_id;
	U8 num_shapers;
	U8 num_sched;
	U8 sched_mask;
        U8 ifg;
	U8 pad[2];

        U16 weight[NUM_QUEUES];
        U16 max_qdepth[NUM_QUEUES];

	QM_ShaperDesc shaper[NUM_SHAPERS];
	QM_SchedDesc  sched[NUM_SCHEDULERS];
	QM_QDesc      q[NUM_QUEUES];
        QM_ShaperDesc_ctl shaper_ctl[NUM_SHAPERS];

	U32 q_len_masks[NUM_QUEUES];
	U8 qresult_regoffset[NUM_QUEUES];
} __attribute__((aligned(32))) QM_context_ctl, *PQM_context_ctl;

extern QM_context g_qm_context;
extern U8 g_qm_cmd_info[QM_CMD_SIZE];
extern U32 tx_trans[NUM_QUEUES];
#endif

#if defined(COMCERTO_2000_CONTROL)

#include "pfe_mod.h"

#define QM_GET_CONTEXT(output_port) (gQMpCtx[output_port])
#define QM_GET_QOSOFF_CONTEXT(output_port) (&gQMQosOffCtx[output_port])

//#define DEFAULT_MAX_QDEPTH 	TMU_LLM_QUEUE_LEN	
#define DEFAULT_HIPRI_MAX_QDEPTH 	0xFF	
#define DEFAULT_LOWPRI_MAX_QDEPTH 	0x1FF

// commands
#define QM_ENABLE 	0x1
#define QM_DISABLE 	0x0

#define	QM_INITIAL_ENABLE_STATE	QM_ENABLE

#define EXPT_PORT_ID	GEM_PORTS

#define EXPT_TYPE_ETH	0x0
#define EXPT_TYPE_WIFI	0x1
#define DEFAULT_EXPT_RATE	1000  /* 10000 packets per msec
					(i.e 1000 packets per usec) */

#define EXPT_CTRLQ_CONFIG	(1 << 0)
#define EXPT_DSCP_CONFIG	(1 << 1)

typedef struct _tQosEnableCommand {
	unsigned short port;
	unsigned short enable_flag;
}QosEnableCommand, *PQosEnableCommand;

typedef struct _tQueueQosEnableCommand {
	unsigned short port;
	unsigned short enable_flag;
	unsigned int queue_qosenable_mask; // Bit mask of queues on which Qos is enabled
}QueueQosEnableCommand, *PQueueQosEnableCommand;

typedef struct _tQosSchedulerCommand {
	unsigned short port;
	unsigned short alg;
}QosSchedulerCommand, *PQosSchedulerCommand;

typedef struct _tQosNhighCommand {
	unsigned short port;
	unsigned short nhigh;
}QosNhighCommand, *PQosNhighCommand;

typedef struct _tQosMaxtxdepthCommand {
	unsigned short port;
	unsigned short maxtxdepth;
}QosMaxtxdepthCommand, *PQosMaxtxdepthCommand;

typedef struct _tQosMaxqdepthCommand {
	unsigned short port;
	unsigned short maxqdepth[NUM_QUEUES];
}QosMaxqdepthCommand, *PQosMaxqdepthCommand;

typedef struct _tQosWeightCommand {
	unsigned short port;
	unsigned short weight[NUM_QUEUES];
}QosWeightCommand, *PQosWeightCommand;

typedef struct _tQosResetCommand {
	unsigned short port;
}QosResetCommand, *PQosResetCommand;

typedef struct _tQosShaperConfigCommand {
	unsigned short port;
	unsigned short shaper_num;
	unsigned short enable_disable_control;
	unsigned char ifg;
	unsigned char ifg_change_flag;
	unsigned int rate;
	unsigned int bucket_size;
	unsigned int qmask;
}QosShaperConfigCommand, *PQosShaperConfigCommand;

typedef struct _tQosSchedulerConfigCommand {
	unsigned short port;
	unsigned short sched_num;
	unsigned char alg;
	unsigned char alg_change_flag;
	unsigned int qmask;
}QosSchedulerConfigCommand, *PQosSchedulerConfigCommand;

typedef struct _tQosExptRateCommand {
	unsigned short expt_iftype; // WIFI or ETH
	unsigned short pkts_per_msec;
}QosExptRateCommand, *PQosExptRateCommand;

// Data structure passed from CMM to QM containing Rate Limiting configuration information
typedef struct _tQosRlCommand {
    U16	port;	// Ethernet Port
    U16    action;   // Rate_Limiting On or Off
    U32    mask;     // bit mask of rate-limited queues attached to this combination
    U32	aggregate_bandwidth; //Configured Aggregate bandwidth in Kbps
    U32 	bucket_size; // Configurable bucket Sizes in bytes 
} QosRlCommand;

typedef struct _tQosRlQuery
{
	unsigned short action;
	unsigned short mask;
	unsigned int   aggregate_bandwidth;
	unsigned int   bucket_size;	

} __attribute__((packed)) QosRlQuery,*pQosRlQuery;

typedef struct _tQosQueryCommand
{
	U16 action;
	U16 port;
	U32 queue_qosenable_mask;         // bit mask of queues on which Qos is enabled
	U32 max_txdepth;

	U32 shaper_qmask[NUM_SHAPERS];			// mask of queues assigned to this shaper
	U32 tokens_per_clock_period[NUM_SHAPERS];	// bits worth of tokens available on every 1 msec clock period
	U32 bucket_size[NUM_SHAPERS];		// max bucket size in bytes 

	U32 sched_qmask[NUM_SCHEDULERS];
	U8 sched_alg[NUM_SCHEDULERS];				// current scheduling algorithm
	
	U16 max_qdepth[NUM_QUEUES];
	

}__attribute__((packed)) QosQueryCmd, *pQosQueryCmd;

int qm_init(void);
void qm_exit(void);

extern PQM_context_ctl gQMpCtx[];
extern QM_context_ctl gQMCtx[];
extern QM_context_ctl gQMExptCtx;
extern QM_context_ctl gQMQosOffCtx[];
extern U16 M_expt_cmdproc(U16 cmd_code, U16 cmd_len, U16 *p);

#endif

#if defined(COMCERTO_2000_TMU)
void tmu_init(void);
void M_QM_process_packet(void);

#elif defined(COMCERTO_2000_CLASS) || defined(COMCERTO_2000_CONTROL) || defined(COMCERTO_2000_UTIL)

#define	NUM_DSCP_VALUES		64

typedef struct _tQoSDSCPQmodCommand {
	unsigned short queue ;
	unsigned short num_dscp;
	unsigned char dscp[NUM_DSCP_VALUES];
}QoSDSCPQmodCommand, *PQoSDSCPQmodCommand;

extern U8 DSCP_to_Qmod[NUM_DSCP_VALUES];
extern U8 DSCP_to_Q[NUM_DSCP_VALUES];
extern U8 g_qos_enable[MAX_PHY_PORTS];

#endif

#endif /* _MODULE_QM_H_ */
