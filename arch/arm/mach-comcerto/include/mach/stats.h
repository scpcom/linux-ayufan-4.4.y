#ifndef _STATS_H_
#define _STATS_H_

#define MAX_BINS		64
#define US_SHIFT		8
#define BYTE_SHIFT		14
#define IO_RATE_SHIFT		2
#define RATE_SHIFT		3

#if defined(CONFIG_COMCERTO_MDMA_PROF)
struct mdma_stats {
	unsigned int time_counter[MAX_BINS];
	unsigned int reqtime_counter[MAX_BINS];
	unsigned int data_counter[MAX_BINS];
	struct timeval last;
	unsigned int init;
};

extern struct mdma_stats mdma_stats;
extern unsigned int mdma_stats_enable;

static inline void comcerto_dma_profiling_start(struct comcerto_dma_sg *sg, unsigned int len)
{
	if (mdma_stats_enable) {
		struct mdma_stats *stats = &mdma_stats;
		unsigned long diff_time_us;
		unsigned int bin;

		do_gettimeofday(&sg->start);

		if (stats->init) {
			diff_time_us = (sg->start.tv_sec - stats->last.tv_sec) * 1000 * 1000 + (sg->start.tv_usec - stats->last.tv_usec);

			bin = diff_time_us >> US_SHIFT;
			if (bin >= MAX_BINS)
				bin = MAX_BINS - 1;

			stats->time_counter[bin]++;
		}

		bin = len >> BYTE_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->data_counter[bin]++;
	}
}

static inline void comcerto_dma_profiling_end(struct comcerto_dma_sg *sg)
{
	if (mdma_stats_enable) {
		struct mdma_stats *stats = &mdma_stats;
		unsigned long diff_time_us;
		unsigned int bin;

		do_gettimeofday(&sg->end);

		diff_time_us = (sg->end.tv_sec - sg->start.tv_sec) * 1000 * 1000 + (sg->end.tv_usec - sg->start.tv_usec);

		bin = diff_time_us >> US_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->reqtime_counter[bin]++;

		if (!stats->init)
			stats->init = 1;

		stats->last = sg->end;
	}
}

#else
#define comcerto_dma_profiling_start(sg, len) do {} while(0)
#define comcerto_dma_profiling_end(sg) do {} while(0)
#endif

#if defined(CONFIG_COMCERTO_SPLICE_PROF)

#include <linux/tcp.h>

struct splicer_stats {
	unsigned int time_counter[MAX_BINS];
	unsigned int reqtime_counter[MAX_BINS];
	unsigned int data_counter[MAX_BINS];
	unsigned int rate_counter[MAX_BINS];
	unsigned int tcp_rsock_counter[MAX_BINS];
	struct timeval last;
	unsigned long active_us;
	unsigned long idle_us;
	unsigned long diff_us;
	unsigned long read;
	unsigned int init;
};

struct splicew_stats {
	unsigned int time_counter[MAX_BINS];
	unsigned int reqtime_counter[MAX_BINS];
	unsigned int data_counter[MAX_BINS];
	unsigned int rate_counter[MAX_BINS];
	struct timeval last;
	unsigned long active_us;
	unsigned long idle_us;
	unsigned long diff_us;
	unsigned long written;
	unsigned int init;
};

extern struct splicer_stats splicer_stats;
extern struct splicew_stats splicew_stats;
extern unsigned int splice_stats_enable;

static inline void splicer_stats_start(void)
{
	if (splice_stats_enable) {
		struct splicer_stats *stats = &splicer_stats;
		struct timeval now;
		unsigned long diff_time_us;
		unsigned int bin;

		do_gettimeofday(&now);

		if (stats->init) {
			diff_time_us = (now.tv_sec - stats->last.tv_sec) * 1000 * 1000 + (now.tv_usec - stats->last.tv_usec);

			bin = diff_time_us >> US_SHIFT;
			if (bin >= MAX_BINS)
				bin = MAX_BINS - 1;

			stats->time_counter[bin]++;

			/* If we were idle for more than one second, it's probably because there was nothing else to write */
			if (diff_time_us < 1000000)
				stats->idle_us += diff_time_us;
		}

		stats->last = now;
	}
}

static inline void splicer_stats_tcp(struct sock *sk)
{
	if (splice_stats_enable) {
		struct splicer_stats *stats = &splicer_stats;
		const struct tcp_sock *tp = tcp_sk(sk);
		unsigned int rsock_qsize = tp->rcv_nxt - tp->copied_seq;
		unsigned int bin;

		bin = rsock_qsize >> BYTE_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->tcp_rsock_counter[bin]++;
	}
}

static inline void splicer_stats_end(ssize_t len)
{
	if (splice_stats_enable) {
		struct splicer_stats *stats = &splicer_stats;
		struct timeval now;
		unsigned long diff_time_us;
		unsigned int bin;

		do_gettimeofday(&now);

		diff_time_us = (now.tv_sec - stats->last.tv_sec) * 1000 * 1000 + (now.tv_usec - stats->last.tv_usec);

		bin = diff_time_us >> US_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->reqtime_counter[bin]++;

		bin = len >> BYTE_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->data_counter[bin]++;

		stats->active_us += diff_time_us;
		stats->diff_us += diff_time_us;
		stats->read += len;

		/* Average rate for every 10MiB */
		if (stats->read > (10 << 20)) {
			bin = (((stats->read / stats->diff_us) * 1000 * 1000) >> 20) >> RATE_SHIFT;
			if (bin >= MAX_BINS)
				bin = MAX_BINS - 1;

			stats->rate_counter[bin] += stats->read >> 10;

			stats->read = 0;
			stats->diff_us = 0;
		}

		if (!stats->init)
			stats->init = 1;

		stats->last = now;
	}
}

static inline void splicew_stats_start(void)
{
	if (splice_stats_enable) {
		struct splicew_stats *stats = &splicew_stats;
		struct timeval now;
		unsigned long diff_time_us;
		unsigned int bin;

		do_gettimeofday(&now);

		if (stats->init) {
			diff_time_us = (now.tv_sec - stats->last.tv_sec) * 1000 * 1000 + (now.tv_usec - stats->last.tv_usec);

			bin = diff_time_us >> US_SHIFT;
			if (bin >= MAX_BINS)
				bin = MAX_BINS - 1;

			stats->time_counter[bin]++;

			/* If we were idle for more than one second, it's probably because there was nothing else to write */
			if (diff_time_us < 1000000)
				stats->idle_us += diff_time_us;
		}

		stats->last = now;
	}
}

static inline void splicew_stats_end(ssize_t len)
{
	if (splice_stats_enable) {
		struct splicew_stats *stats = &splicew_stats;
		struct timeval now;
		unsigned long diff_time_us;
		unsigned int bin;

		do_gettimeofday(&now);

		diff_time_us = (now.tv_sec - stats->last.tv_sec) * 1000 * 1000 + (now.tv_usec - stats->last.tv_usec);

		bin = diff_time_us >> US_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->reqtime_counter[bin]++;

		bin = len >> BYTE_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->data_counter[bin]++;

		stats->active_us += diff_time_us;
		stats->diff_us += diff_time_us;
		stats->written += len;

		/* Average rate for every 10MiB */
		if (stats->written > (10 << 20)) {
			bin = (((stats->written / stats->diff_us) * 1000 * 1000) >> 20) >> RATE_SHIFT;
			if (bin >= MAX_BINS)
				bin = MAX_BINS - 1;

			stats->rate_counter[bin] += stats->written >> 10;

			stats->written = 0;
			stats->diff_us = 0;
		}

		if (!stats->init)
			stats->init = 1;

		stats->last = now;
	}
}
#else
#define splicer_stats_start() do {} while(0)
#define splicer_stats_tcp(sk) do {} while(0)
#define splicer_stats_end(len) do {} while(0)

#define splicew_stats_start() do {} while(0)
#define splicew_stats_end(len) do {} while(0)
#endif

#if defined(CONFIG_COMCERTO_AHCI_PROF)

#define MAX_AHCI_PORTS		4
#define MAX_AHCI_SLOTS		32

struct ahci_port_stats {
	struct timeval first_issue;
	struct timeval last_issue;

	unsigned long active_us;
	unsigned long idle_us;

	unsigned long read_kbytes_total;
	unsigned long write_kbytes_total;
	unsigned long other_kbytes_total;

	unsigned long read_total;
	unsigned long write_total;
	unsigned long other_total;

	unsigned int pending_flag;
	unsigned int nb_pending;
	unsigned int nb_pending_max;
	unsigned int nb_pending_total;
	unsigned int bytes_pending;
	unsigned long diff_us;
	unsigned int pending_counter[MAX_BINS];
	unsigned int rate_counter[MAX_BINS];

	unsigned int init;
	unsigned int time_counter[MAX_BINS]; // 128us -> 16ms
	unsigned int data_counter[MAX_BINS]; // 4K-> 1020K
	struct timeval last_req;
};

extern struct ahci_port_stats ahci_port_stats[MAX_AHCI_PORTS];
extern unsigned int ahci_stats_enable;

#include <linux/ata.h>
#include <linux/libata.h>

static inline void ahci_stats_start(struct ata_port *ap, struct ata_queued_cmd *qc)
{
	if (ahci_stats_enable) {
		struct ahci_port_stats *stats = &ahci_port_stats[ap->port_no];
		struct timeval now;
		unsigned long diff_time_us;
		unsigned int bin;

		do_gettimeofday(&now);

		if (stats->init) {
			diff_time_us = (now.tv_sec - stats->last_req.tv_sec) * 1000 * 1000 + (now.tv_usec - stats->last_req.tv_usec);

			bin = diff_time_us >> US_SHIFT;
			if (bin >= MAX_BINS)
				bin = MAX_BINS - 1;

			stats->time_counter[bin]++;
		} else {
			stats->init = 1;
		}

		stats->last_req = now;

		bin = qc->nbytes >> BYTE_SHIFT;
		if (bin >= MAX_BINS)
			bin = MAX_BINS - 1;

		stats->data_counter[bin]++;

		if (!stats->nb_pending) {

			diff_time_us = (now.tv_sec - stats->last_issue.tv_sec) * 1000 * 1000 +
					(now.tv_usec - stats->last_issue.tv_usec);

			/* If we were idle for more than one second, it's probably because there was nothing else to write */
			/* If there is not active time, then we are coming out of idle state */
			if ((diff_time_us < 1000000) && stats->active_us)
				stats->idle_us += diff_time_us;

			stats->first_issue = now;
			stats->nb_pending_total = 0;
		}

		stats->nb_pending_total++;

		/* This should never overflow */
		stats->pending_counter[stats->nb_pending & (MAX_AHCI_SLOTS - 1)]++;

		stats->nb_pending++;

		if (stats->nb_pending_total > stats->nb_pending_max)
			stats->nb_pending_max = stats->nb_pending_total;

		stats->bytes_pending += qc->nbytes;
		stats->pending_flag |= 1 << qc->tag;
	}
}

static inline void ahci_stats_end(struct ata_port *ap, struct ata_queued_cmd *qc, unsigned int tag)
{
	if (ahci_stats_enable) {
		struct ahci_port_stats *stats = &ahci_port_stats[ap->port_no];

		if (stats->pending_flag & (1 << tag)) {
			stats->pending_flag &= ~(1 << tag);
			stats->nb_pending--;

			switch (qc->tf.command) {
			case ATA_CMD_FPDMA_READ:
			case ATA_CMD_READ:
			case ATA_CMD_READ_EXT:
				stats->read_kbytes_total += qc->nbytes >> 10;
				stats->read_total++;
				break;

			case ATA_CMD_FPDMA_WRITE:
			case ATA_CMD_WRITE:
			case ATA_CMD_WRITE_EXT:
				stats->write_kbytes_total += qc->nbytes >> 10;
				stats->write_total++;
				break;

			default:
				stats->other_kbytes_total += qc->nbytes >> 10;
				stats->other_total++;

				break;
			}

			if (!stats->nb_pending) {
				struct timeval now;
				unsigned long diff_time_us;
				unsigned int rate;
				unsigned int bin;

				do_gettimeofday(&now);

				diff_time_us = (now.tv_sec - stats->first_issue.tv_sec) * 1000 * 1000 +
						(now.tv_usec - stats->first_issue.tv_usec);

				stats->last_issue = now;

				stats->active_us += diff_time_us;

				stats->diff_us += diff_time_us;

				/* Do the average for at least 10MiB of data transfered */
				if (stats->bytes_pending > (10 * (1 << 20))) {

					rate = ((stats->bytes_pending / stats->diff_us) * 1000 * 1000) >> 20; //MiBps

					bin = rate >> IO_RATE_SHIFT;
					if (bin >= MAX_BINS)
						bin = MAX_BINS - 1;

					/* Track how many KiB were transfered at this rate */
					stats->rate_counter[bin] += stats->bytes_pending >> 10;

					/* Reset stats */
					stats->bytes_pending = 0;
					stats->diff_us = 0;
				}
			}
		}
	}
}
#else
#define ahci_stats_start(ap, qc) do {} while(0)
#define ahci_stats_end(ap, qc, tag) do {} while(0)
#endif

#endif /* _STATS_H_ */
