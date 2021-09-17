#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/of.h>
#include <linux/init.h>
#include <linux/bpa2.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/platform_data/dma-st-fdma.h>
#include <sound/core.h>
#include "common.h"
#include "reg_aud_uniperif.h"

#ifndef CONFIG_BPA2
#error "BPA2 must be configured for Uniperif TDM driver"
#endif

#define UNIPERIF_TDM_MIN_HANDSETS	1
#define UNIPERIF_TDM_MAX_HANDSETS	10

#define UNIPERIF_TDM_MIN_PERIODS	4
#define UNIPERIF_TDM_DEF_PERIODS	10

#define UNIPERIF_TDM_MIN_TIMESLOTS	1
#define UNIPERIF_TDM_MAX_TIMESLOTS	128

#define UNIPERIF_TDM_FREQ_8KHZ		8000
#define UNIPERIF_TDM_FREQ_16KHZ		16000
#define UNIPERIF_TDM_FREQ_32KHZ		32000

#define UNIPERIF_TDM_HW_INFO		(SNDRV_PCM_INFO_MMAP | \
					 SNDRV_PCM_INFO_MMAP_VALID | \
					 SNDRV_PCM_INFO_INTERLEAVED | \
					 SNDRV_PCM_INFO_BLOCK_TRANSFER)

#define UNIPERIF_TDM_HW_FORMAT_CNB	(SNDRV_PCM_FMTBIT_S8 | \
					SNDRV_PCM_FMTBIT_U8)
#define UNIPERIF_TDM_HW_FORMAT_LNB	SNDRV_PCM_FMTBIT_S16_LE
#define UNIPERIF_TDM_HW_FORMAT_CWB	SNDRV_PCM_FMTBIT_S16_LE
#define UNIPERIF_TDM_HW_FORMAT_LWB	SNDRV_PCM_FMTBIT_S32_LE

#define UNIPERIF_TDM_BUF_OFF_MAX	((1UL << (14 + 3)) - 1)

#define UNIPERIF_TDM_DMA_MAXBURST	35   

#define UNIPERIF_TDM_FIFO_ERROR_LIMIT	2   

#define SND_STM_TELSS_HANDSET_INFO(fs, s1, s2, v, dup, dat, cn, ln, cw, lw)\
	{ \
		.fsync = fs, \
		.slot1 = s1, \
		.slot2 = s2, \
		.slot2_valid = v, \
		.duplicate = dup, \
		.data16 = dat, \
		.cnb = cn, \
		.lnb = ln, \
		.cwb = cw, \
		.lwb = lw, \
	}

#define UNIPERIF_TDM_DUMP_REGISTER(r) \
		snd_iprintf(buffer, "AUD_UNIPERIF_%-23s (offset 0x%04x) = " \
				"0x%08x\n", __stringify(r), \
				offset__AUD_UNIPERIF_##r(tdm), \
				get__AUD_UNIPERIF_##r(tdm))

struct snd_stm_telss_handset_info {
	unsigned int fsync;
	unsigned int slot1;
	unsigned int slot2;
	unsigned int slot2_valid;
	unsigned int duplicate;
	unsigned int data16;
	unsigned int cnb;
	unsigned int lnb;
	unsigned int cwb;
	unsigned int lwb;
};

struct snd_stm_telss_word_pos_info {
	unsigned int msb;
	unsigned int lsb;
};

struct snd_stm_telss_timeslot_info {
	int word_num;
	struct snd_stm_telss_word_pos_info *word_pos;
};

struct snd_stm_uniperif_tdm_info {
	const char *name;
	int ver;
	int card_device;

	const char *fdma_name;
	struct device_node *dma_np;
	unsigned int fdma_channel;
	unsigned int fdma_initiator;
	enum dma_transfer_direction fdma_direction;
	unsigned int fdma_direct_conn;
	unsigned int fdma_request_line;

	unsigned int rising_edge;		 
	unsigned int clk_rate;			 
	unsigned int pclk_rate;			 
	unsigned int fs_rate;			 
	unsigned int timeslots;			 
	unsigned int fs01_rate;			 
	unsigned int fs02_rate;			 
	unsigned int fs02_delay_clock;		 
	unsigned int fs02_delay_timeslot;	 
	unsigned int msbit_start;		 
	struct snd_stm_telss_timeslot_info *timeslot_info;

	unsigned int frame_size;		 
	unsigned int frame_count;		 
	unsigned int max_periods;		 
	unsigned int handset_count;		 
	struct snd_stm_telss_handset_info *handset_info;
};

struct uniperif_tdm;

struct telss_handset {
	unsigned int id;			 
	unsigned int ctl;			 
	struct snd_pcm *pcm;			 
	struct uniperif_tdm *tdm;		 
	struct snd_pcm_hardware hw;		 
	struct snd_pcm_substream *substream;	 

	unsigned int hw_xrun;			 
	unsigned int call_xrun;			 
	unsigned int call_valid;		 
	unsigned int period_max_sz;		 
	unsigned int buffer_max_sz;		 
	unsigned int buffer_act_sz;		 
	unsigned int buffer_offset;		 
	unsigned int dma_period;		 

	struct snd_stm_telss_handset_info *info;
	struct st_dma_telss_handset_config config;

	snd_stm_magic_field;
};

struct uniperif_tdm {
	struct device *dev;
	struct snd_stm_uniperif_tdm_info *info;
	int ver;  

	struct resource *mem_region;
	void __iomem *base;
	unsigned int irq;
	struct snd_stm_clk *clk;		 
	struct snd_stm_clk *pclk;		 
	spinlock_t lock;			 

	struct snd_info_entry *proc_entry;

	int open_ref;				 
	int start_ref;				 

	struct bpa2_part *buffer_bpa2;		 
	dma_addr_t buffer_phys;			 
	unsigned char *buffer_area;		 
	size_t buffer_max_sz;			 
	size_t buffer_act_sz;			 

	size_t period_max_sz;			 
	int period_max_cnt;			 
	int period_act_cnt;			 

	struct mutex dma_mutex;			 
	dma_cookie_t dma_cookie;
	unsigned int dma_maxburst;
	struct dma_chan *dma_channel;
	struct st_dma_telss_config dma_config;
	struct dma_async_tx_descriptor *dma_descriptor;
	unsigned int dma_fifo_errors;		 
	s64 dma_last_callback_time;		 

	struct telss_handset handsets[UNIPERIF_TDM_MAX_HANDSETS];

	snd_stm_magic_field;
};

static unsigned int uniperif_tdm_handset_pcm_ctl;

struct uniperif_tdm_mem_fmt_map {
	unsigned int frame_size;
	unsigned int channels;
	unsigned int mem_fmt;
};

static void uniperif_tdm_hw_reset(struct uniperif_tdm *tdm);
static void uniperif_tdm_hw_enable(struct uniperif_tdm *tdm);
static void uniperif_tdm_hw_disable(struct uniperif_tdm *tdm);
static void uniperif_tdm_hw_start(struct uniperif_tdm *tdm);
static void uniperif_tdm_hw_stop(struct uniperif_tdm *tdm);

static void uniperif_tdm_xrun(struct uniperif_tdm *tdm, const char *error)
{
	unsigned long flags;
	int i;

	dev_err(tdm->dev, error);

	spin_lock_irqsave(&tdm->lock, flags);

	for (i = 0; i < tdm->info->handset_count; ++i)
		if (tdm->handsets[i].substream)
			if (tdm->handsets[i].call_valid) {
				 
				tdm->handsets[i].hw_xrun = 1;

				spin_unlock_irqrestore(&tdm->lock, flags);
				snd_pcm_stop(tdm->handsets[i].substream,
						SNDRV_PCM_STATE_XRUN);
				spin_lock_irqsave(&tdm->lock, flags);
			}

	spin_unlock_irqrestore(&tdm->lock, flags);
}

static irqreturn_t uniperif_tdm_irq_handler(int irq, void *dev_id)
{
	struct uniperif_tdm *tdm = dev_id;
	irqreturn_t result = IRQ_NONE;
	unsigned int status;

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	preempt_disable();
	status = get__AUD_UNIPERIF_ITS(tdm);
	set__AUD_UNIPERIF_ITS_BCLR(tdm, status);
	preempt_enable();

	if (unlikely(status & mask__AUD_UNIPERIF_ITS__FIFO_ERROR(tdm))) {
		 
		if (++tdm->dma_fifo_errors > UNIPERIF_TDM_FIFO_ERROR_LIMIT) {
			 
			set__AUD_UNIPERIF_ITM_BCLR__FIFO_ERROR(tdm);

			uniperif_tdm_xrun(tdm, "FIFO error!");
		}

		result = IRQ_HANDLED;

	} else if (unlikely(status & mask__AUD_UNIPERIF_ITS__DMA_ERROR(tdm))) {
		 
		set__AUD_UNIPERIF_ITM_BCLR__DMA_ERROR(tdm);

		uniperif_tdm_xrun(tdm, "DMA error!");

		result = IRQ_HANDLED;
	}

	if (result != IRQ_HANDLED)
		dev_err(tdm->dev, "Unhandled IRQ: %08x", status);

	return result;
}

static bool uniperif_tdm_dma_filter_fn(struct dma_chan *chan, void *fn_param)
{
	struct uniperif_tdm *tdm = fn_param;
	struct st_dma_telss_config *config = &tdm->dma_config;

	if (tdm->info->dma_np)
		if (tdm->info->dma_np != chan->device->dev->of_node)
			return false;

	if (tdm->info->fdma_channel)
		if (!st_dma_is_fdma_channel(chan, tdm->info->fdma_channel))
			return false;

	config->type = ST_DMA_TYPE_TELSS;
	config->dma_addr = tdm->mem_region->start +
			offset__AUD_UNIPERIF_FIFO_DATA(tdm);

	config->dreq_config.request_line = tdm->info->fdma_request_line;
	config->dreq_config.direct_conn = tdm->info->fdma_direct_conn;
	config->dreq_config.initiator = tdm->info->fdma_initiator;
	config->dreq_config.increment = 0;
	config->dreq_config.hold_off = 0;
	config->dreq_config.maxburst = tdm->dma_maxburst;
	config->dreq_config.buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
	config->dreq_config.direction = tdm->info->fdma_direction;

	config->frame_size = tdm->info->frame_size - 1;  
	config->frame_count = tdm->info->frame_count;
	config->handset_count = tdm->info->handset_count;

	chan->private = config;

	dev_dbg(tdm->dev, "Using '%s' channel %d",
			dev_name(chan->device->dev), chan->chan_id);

	return true;
}

static int uniperif_tdm_dma_request(struct uniperif_tdm *tdm)
{
	dma_cap_mask_t mask;
	int result;
	int i;

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	mutex_lock(&tdm->dma_mutex);

	if (tdm->dma_channel) {
		mutex_unlock(&tdm->dma_mutex);
		return 0;
	}

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_cap_set(DMA_CYCLIC, mask);

	tdm->dma_channel = dma_request_channel(mask,
			uniperif_tdm_dma_filter_fn, tdm);
	if (!tdm->dma_channel) {
		dev_err(tdm->dev, "Failed to request DMA channel");
		mutex_unlock(&tdm->dma_mutex);
		return -ENODEV;
	}

	for (i = 0; i < tdm->info->handset_count; ++i) {
		struct telss_handset *handset = &tdm->handsets[i];
		struct st_dma_telss_handset_config *config = &handset->config;

		config->buffer_offset = handset->buffer_offset;
		config->period_offset = 0;
		config->period_stride = handset->period_max_sz;

		config->first_slot_id = handset->info->slot1;
		config->second_slot_id = handset->info->slot2;
		config->second_slot_id_valid = handset->info->slot2_valid;
		config->duplicate_enable = handset->info->duplicate;
		config->data_length = handset->info->data16;
		config->call_valid = false;

		result = dma_telss_handset_config(tdm->dma_channel, handset->id,
				config);
		if (result) {
			dev_err(tdm->dev, "Failed to configure handset %d", i);
			dma_release_channel(tdm->dma_channel);
			mutex_unlock(&tdm->dma_mutex);
			return result;
		}
	}

	mutex_unlock(&tdm->dma_mutex);
	return 0;
}

static void uniperif_tdm_dma_release(struct uniperif_tdm *tdm)
{
	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	mutex_lock(&tdm->dma_mutex);

	if (tdm->dma_channel) {
		 
		dmaengine_terminate_all(tdm->dma_channel);

		dma_release_channel(tdm->dma_channel);
		tdm->dma_channel = NULL;
	}

	mutex_unlock(&tdm->dma_mutex);
}

static int uniperif_tdm_open(struct snd_pcm_substream *substream)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct uniperif_tdm *tdm;
	unsigned long flags;
	int result;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	tdm = handset->tdm;

	dev_dbg(tdm->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(handset->substream);

	snd_pcm_set_sync(substream);

	result = uniperif_tdm_dma_request(tdm);
	if (result) {
		dev_err(tdm->dev, "Failed to request DMA channel");
		return result;
	}

	result = snd_pcm_hw_constraint_minmax(runtime,
			SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
			tdm->info->frame_count, tdm->info->frame_count);
	if (result < 0) {
		dev_err(tdm->dev, "Failed to constrain period size");
		return result;
	}

	handset->hw.info = UNIPERIF_TDM_HW_INFO;

	if (handset->info->cnb)
		handset->hw.formats |= UNIPERIF_TDM_HW_FORMAT_CNB;
	 
	if (handset->info->lnb)
		handset->hw.formats |= UNIPERIF_TDM_HW_FORMAT_LNB;
	 
	if (handset->info->cwb)
		handset->hw.formats |= UNIPERIF_TDM_HW_FORMAT_CWB;
	 
	if (handset->info->lwb)
		handset->hw.formats |= UNIPERIF_TDM_HW_FORMAT_LWB;

	handset->hw.rates = SNDRV_PCM_RATE_CONTINUOUS,
	handset->hw.rate_min = handset->info->fsync;
	handset->hw.rate_max = handset->info->fsync;

	handset->hw.channels_min = 1;
	handset->hw.channels_max = 1;

	spin_lock_irqsave(&tdm->lock, flags);

	if (++tdm->open_ref == 1) {
		 
		handset->hw.periods_min	= UNIPERIF_TDM_MIN_PERIODS;
		handset->hw.periods_max	= tdm->period_max_cnt;

		handset->hw.buffer_bytes_max = handset->buffer_max_sz;
	} else {
		 
		handset->hw.periods_min	= tdm->period_act_cnt;
		handset->hw.periods_max	= tdm->period_act_cnt;

		handset->hw.buffer_bytes_max = handset->buffer_act_sz;
	}

	spin_unlock_irqrestore(&tdm->lock, flags);

	handset->hw.period_bytes_min = 1;
	handset->hw.period_bytes_max = handset->period_max_sz;

	substream->runtime->hw = handset->hw;

	handset->substream = substream;

	return 0;
}

static int uniperif_tdm_close(struct snd_pcm_substream *substream)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	struct uniperif_tdm *tdm;
	unsigned long flags;
	int i;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	tdm = handset->tdm;

	dev_dbg(tdm->dev, "%s(substream=%p)", __func__, substream);

	spin_lock_irqsave(&tdm->lock, flags);

	if (handset->call_xrun) {
		 
		if (--tdm->start_ref == 0)
			uniperif_tdm_hw_stop(tdm);

		handset->call_xrun = 0;
	}

	handset->substream = NULL;

	if (--tdm->open_ref == 1) {
		 
		for (i = 0; i < tdm->info->handset_count; ++i) {
			 
			if (tdm->handsets[i].substream == NULL)
				continue;

			tdm->handsets[i].hw.periods_min =
					UNIPERIF_TDM_MIN_PERIODS;
			tdm->handsets[i].hw.periods_max	= tdm->period_max_cnt;

			tdm->handsets[i].hw.buffer_bytes_max =
					tdm->handsets[i].buffer_max_sz;
		}
	}

	spin_unlock_irqrestore(&tdm->lock, flags);

	return 0;
}

static int uniperif_tdm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct uniperif_tdm *tdm;
	unsigned long flags;
	int buffer_new_sz;
	int period_new_cnt;
	int i;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!runtime);
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	dev_dbg(handset->tdm->dev, "%s(substream=%p, hw_params=%p)", __func__,
		   substream, hw_params);

	if ((runtime->dma_addr != 0) && (runtime->dma_area != NULL))
		return 0;

	tdm = handset->tdm;

	buffer_new_sz = params_buffer_size(hw_params);

	period_new_cnt = buffer_new_sz / tdm->info->frame_count;

	if (buffer_new_sz % tdm->info->frame_count) {
		dev_err(tdm->dev, "Buffer size not multiple of period size");
		return -EINVAL;
	}

	spin_lock_irqsave(&tdm->lock, flags);

	if (period_new_cnt != tdm->period_act_cnt) {
		 
		if (tdm->open_ref > 1) {
			dev_err(tdm->dev, "More than 1 handset open");
			spin_unlock_irqrestore(&tdm->lock, flags);
			return -EINVAL;
		}

		BUG_ON(tdm->period_act_cnt > tdm->period_max_cnt);

		if (tdm->period_act_cnt < UNIPERIF_TDM_MIN_PERIODS) {
			dev_err(tdm->dev, "Buffer size must be > 2 periods");
			spin_unlock_irqrestore(&tdm->lock, flags);
			return -EINVAL;
		}

		tdm->period_act_cnt = period_new_cnt;
		tdm->buffer_act_sz = tdm->period_max_sz * tdm->period_act_cnt;

		for (i = 0; i < tdm->info->handset_count; ++i)
			tdm->handsets[i].buffer_act_sz =
				tdm->buffer_act_sz / tdm->info->handset_count;
	}

	spin_unlock_irqrestore(&tdm->lock, flags);

	runtime->dma_addr = handset->tdm->buffer_phys + handset->buffer_offset;
	runtime->dma_area = handset->tdm->buffer_area + handset->buffer_offset;
	runtime->dma_bytes = handset->buffer_act_sz;

	return 0;
}

static int uniperif_tdm_hw_free(struct snd_pcm_substream *substream)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!runtime);
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	dev_dbg(handset->tdm->dev, "%s(substream=%p)", __func__, substream);

	if ((runtime->dma_addr == 0) && (runtime->dma_area == NULL))
		return 0;

	runtime->dma_addr = 0;
	runtime->dma_area = NULL;
	runtime->dma_bytes = 0;

	return 0;
}

static int uniperif_tdm_prepare(struct snd_pcm_substream *substream)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct uniperif_tdm *tdm;
	int result;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!runtime);
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	tdm = handset->tdm;

	dev_dbg(tdm->dev, "%s(substream=%p)", __func__, substream);

	switch (substream->runtime->format) {
	case SNDRV_PCM_FORMAT_S8:
	case SNDRV_PCM_FORMAT_U8:
		dev_dbg(tdm->dev, "SNDRV_PCM_FORMAT_S8/U8");
		handset->config.second_slot_id_valid = false;
		handset->config.duplicate_enable = false;
		handset->config.data_length = false;
		handset->config.period_stride = tdm->info->frame_count;
		break;

	case SNDRV_PCM_FORMAT_S16_LE:
		dev_dbg(tdm->dev, "SNDRV_PCM_FORMAT_S16_LE");
		handset->config.second_slot_id_valid = false;
		handset->config.duplicate_enable = false;
		handset->config.data_length = true;
		handset->config.period_stride = tdm->info->frame_count * 2;
		break;

	case SNDRV_PCM_FORMAT_S32_LE:
		dev_dbg(tdm->dev, "SNDRV_PCM_FORMAT_S32_LE");
		handset->config.second_slot_id_valid = true;
		handset->config.duplicate_enable = false;
		handset->config.data_length = true;
		handset->config.period_stride = tdm->info->frame_count * 4;
		break;

	default:
		dev_err(tdm->dev, "Unsupported audio format %d",
				substream->runtime->format);
		BUG();
		return -EINVAL;
	}

	result = dma_telss_handset_config(tdm->dma_channel, handset->id,
			&handset->config);
	if (result) {
		dev_err(tdm->dev, "Failed to configure handset");
		return result;
	}

	return 0;
}

static void uniperif_tdm_dma_callback(void *param)
{
	struct uniperif_tdm *tdm = param;
	unsigned long flags;
	int period;
	s64 now;
	int i;

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	now = ktime_to_us(ktime_get());

	period = dma_telss_get_period(tdm->dma_channel);

	tdm->dma_last_callback_time = now;

	for (i = 0; i < tdm->info->handset_count; ++i) {
		struct telss_handset *handset = &tdm->handsets[i];

		spin_lock_irqsave(&tdm->lock, flags);

		if (!handset->substream) {
			spin_unlock_irqrestore(&tdm->lock, flags);
			continue;
		}

		if (!handset->call_valid) {
			spin_unlock_irqrestore(&tdm->lock, flags);
			continue;
		}

		handset->dma_period = period;
		handset->dma_period -= handset->config.period_offset;
		handset->dma_period += tdm->period_act_cnt;
		handset->dma_period %= tdm->period_act_cnt;

		spin_unlock_irqrestore(&tdm->lock, flags);

		snd_pcm_period_elapsed(handset->substream);
	}
}

static int uniperif_tdm_start(struct snd_pcm_substream *substream)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	struct uniperif_tdm *tdm;
	int result;
	int new_period_offset;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	tdm = handset->tdm;

	dev_dbg(tdm->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(handset->call_valid);

	if (tdm->info->fdma_direction == DMA_DEV_TO_MEM)
		memset(substream->runtime->dma_area, 0, handset->buffer_act_sz);

	handset->dma_period = 0;

	if (tdm->start_ref == 0) {
		 
		handset->config.period_offset = 0;

		if (tdm->info->fdma_direction == DMA_DEV_TO_MEM)
			handset->config.call_valid = true;

		result = dma_telss_handset_config(tdm->dma_channel, handset->id,
				&handset->config);
		if (result) {
			dev_err(tdm->dev, "Failed to set handset config");
			goto error_handset_config;
		}

		tdm->dma_descriptor = dma_telss_prep_dma_cyclic(
				tdm->dma_channel,
				tdm->buffer_phys,
				tdm->buffer_act_sz,
				tdm->period_max_sz,
				tdm->info->fdma_direction);
		if (!tdm->dma_descriptor) {
			dev_err(tdm->dev, "Failed to prepare dma descriptor");
			result = -ENOMEM;
			goto error_prep_dma_cyclic;
		}

		tdm->dma_descriptor->callback = uniperif_tdm_dma_callback;
		tdm->dma_descriptor->callback_param = tdm;

		uniperif_tdm_hw_start(tdm);

		if (tdm->info->fdma_direction == DMA_MEM_TO_DEV)
			handset->config.period_offset = 1;
	} else {
		 
		handset->config.period_offset =
				dma_telss_get_period(tdm->dma_channel) + 1;

		handset->config.period_offset %= tdm->period_act_cnt;
	}

	new_period_offset =  handset->config.period_offset;

	if (!handset->config.call_valid) {
		 
		handset->config.call_valid = true;

		do {
			handset->config.period_offset = new_period_offset;

			result = dma_telss_handset_config(tdm->dma_channel,
					handset->id, &handset->config);
			if (result) {
				dev_err(tdm->dev, "Failed to set handset config");
				goto error_handset_config;
			}

			 if (tdm->start_ref == 0)
				break;

			new_period_offset =
				dma_telss_get_period(tdm->dma_channel) + 1;

			new_period_offset %= tdm->period_act_cnt;

		} while (new_period_offset != handset->config.period_offset);

	}

	if (!handset->call_xrun)
		tdm->start_ref++;

	handset->call_xrun = 0;
	handset->call_valid = 1;

	return 0;

error_prep_dma_cyclic:
	dma_telss_handset_control(tdm->dma_channel, handset->id, 0);
error_handset_config:
	handset->config.call_valid = false;
	return result;
}

static int uniperif_tdm_stop(struct snd_pcm_substream *substream)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	struct uniperif_tdm *tdm;
	int result;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	tdm = handset->tdm;

	dev_dbg(tdm->dev, "%s(substream=%p)", __func__, substream);

	if (!handset->call_valid) {
		dev_err(tdm->dev, "Stop called when call not valid!");
		return 0;
	}

	handset->config.period_offset = 0;
	handset->config.call_valid = false;

	result = dma_telss_handset_config(tdm->dma_channel, handset->id,
			&handset->config);
	if (result) {
		dev_err(tdm->dev, "Failed to set handset config");
		return result;
	}

	if (tdm->info->fdma_direction == DMA_MEM_TO_DEV)
		memset(substream->runtime->dma_area, 0, handset->buffer_act_sz);

	if (substream->runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
		 
		if (!handset->hw_xrun) {
			handset->call_xrun = 1;
			handset->call_valid = 0;
			return 0;
		}
	}

	if (--tdm->start_ref == 0)
		uniperif_tdm_hw_stop(tdm);

	handset->hw_xrun = 0;
	handset->call_xrun = 0;
	handset->call_valid = 0;

	return 0;
}

static int uniperif_tdm_trigger(struct snd_pcm_substream *substream,
		int command)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);
	unsigned long flags;
	int result;

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	spin_lock_irqsave(&handset->tdm->lock, flags);

	switch (command) {
	case SNDRV_PCM_TRIGGER_START:
		result = uniperif_tdm_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		result = uniperif_tdm_stop(substream);
		break;
	default:
		result = -EINVAL;
		BUG();
	}

	spin_unlock_irqrestore(&handset->tdm->lock, flags);

	return result;
}

static snd_pcm_uframes_t uniperif_tdm_pointer(
		struct snd_pcm_substream *substream)
{
	struct telss_handset *handset = snd_pcm_substream_chip(substream);

	BUG_ON(!handset);
	BUG_ON(!snd_stm_magic_valid(handset));
	BUG_ON(!handset->tdm);
	BUG_ON(!snd_stm_magic_valid(handset->tdm));

	BUG_ON(!handset->call_valid);

	return handset->dma_period * handset->tdm->info->frame_count;
}

static struct snd_pcm_ops uniperif_tdm_pcm_ops = {
	.open		= uniperif_tdm_open,
	.close		= uniperif_tdm_close,
	.mmap		= snd_stm_buffer_mmap,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= uniperif_tdm_hw_params,
	.hw_free	= uniperif_tdm_hw_free,
	.prepare	= uniperif_tdm_prepare,
	.trigger	= uniperif_tdm_trigger,
	.pointer	= uniperif_tdm_pointer,
};

static int uniperif_tdm_lookup_mem_fmt(struct uniperif_tdm *tdm,
		unsigned int *mem_fmt, unsigned int *channels)
{
	static struct uniperif_tdm_mem_fmt_map mem_fmt_table[] = {
		 
		{1, 2, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_16(NULL)},
		{2, 4, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_16(NULL)},
		{3, 6, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_16(NULL)},
		{4, 8, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_16(NULL)},
		 
		{2, 2, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(NULL)},
		{4, 4, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(NULL)},
		{6, 6, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(NULL)},
		{8, 8, value__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(NULL)},
	};
	static int mem_fmt_table_size = ARRAY_SIZE(mem_fmt_table);
	int i;

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));
	BUG_ON(!mem_fmt);
	BUG_ON(!channels);

	for (i = 0; i < mem_fmt_table_size; ++i) {
		 
		if (tdm->info->frame_size != mem_fmt_table[i].frame_size)
			continue;

		*mem_fmt = mem_fmt_table[i].mem_fmt;
		*channels = mem_fmt_table[i].channels / 2;
		return 0;
	}

	return -EINVAL;
}

static int uniperif_tdm_set_freqs(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	if (tdm->info->fdma_direction == DMA_DEV_TO_MEM)
		return 0;

	switch (tdm->info->fs_rate) {
	case UNIPERIF_TDM_FREQ_8KHZ:
		set__AUD_UNIPERIF_TDM_FS_REF_FREQ__8KHZ(tdm);
		break;

	case UNIPERIF_TDM_FREQ_16KHZ:
		set__AUD_UNIPERIF_TDM_FS_REF_FREQ__16KHZ(tdm);
		break;

	case UNIPERIF_TDM_FREQ_32KHZ:
		set__AUD_UNIPERIF_TDM_FS_REF_FREQ__32KHZ(tdm);
		break;

	default:
		dev_err(tdm->dev, "Invalid fs_ref (%d)", tdm->info->fs_rate);
		return -EINVAL;
	}

	BUG_ON(tdm->info->timeslots < UNIPERIF_TDM_MIN_TIMESLOTS);
	BUG_ON(tdm->info->timeslots > UNIPERIF_TDM_MAX_TIMESLOTS);

	set__AUD_UNIPERIF_TDM_FS_REF_DIV__NUM_TIMESLOT(tdm,
			tdm->info->timeslots);

	BUG_ON(tdm->info->fs01_rate > tdm->info->fs_rate);

	switch (tdm->info->fs01_rate) {
	case UNIPERIF_TDM_FREQ_8KHZ:
		set__AUD_UNIPERIF_TDM_FS01_FREQ__8KHZ(tdm);
		set__AUD_UNIPERIF_TDM_FS01_WIDTH__1BIT(tdm);
		break;

	case UNIPERIF_TDM_FREQ_16KHZ:
		set__AUD_UNIPERIF_TDM_FS_REF_FREQ__16KHZ(tdm);
		set__AUD_UNIPERIF_TDM_FS01_FREQ__16KHZ(tdm);
		set__AUD_UNIPERIF_TDM_FS01_WIDTH__1BIT(tdm);
		break;

	default:
		dev_err(tdm->dev, "Invalid fs01 (%d)", tdm->info->fs01_rate);
		return -EINVAL;
	}

	BUG_ON(tdm->info->fs02_rate > tdm->info->fs_rate);

	switch (tdm->info->fs02_rate) {
	case UNIPERIF_TDM_FREQ_8KHZ:
		set__AUD_UNIPERIF_TDM_FS02_FREQ__8KHZ(tdm);
		set__AUD_UNIPERIF_TDM_FS02_WIDTH__1BIT(tdm);
		break;

	case UNIPERIF_TDM_FREQ_16KHZ:
		set__AUD_UNIPERIF_TDM_FS02_FREQ__16KHZ(tdm);
		set__AUD_UNIPERIF_TDM_FS02_WIDTH__1BIT(tdm);
		break;

	default:
		dev_err(tdm->dev, "Invalid fs02 (%d)", tdm->info->fs02_rate);
		return -EINVAL;
	}

	set__AUD_UNIPERIF_TDM_FS02_TIMESLOT_DELAY__PCM_CLOCK(tdm,
			tdm->info->fs02_delay_clock);
	set__AUD_UNIPERIF_TDM_FS02_TIMESLOT_DELAY__TIMESLOT(tdm,
			tdm->info->fs02_delay_timeslot);

	return 0;
}

static void uniperif_tdm_configure_timeslots(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	set__AUD_UNIPERIF_TDM_DATA_MSBIT_START__DELAY(tdm,
			tdm->info->msbit_start);
}

static int uniperif_tdm_configure(struct uniperif_tdm *tdm)
{
	unsigned int mem_fmt;
	unsigned int channels;
	int result;

	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	result = uniperif_tdm_lookup_mem_fmt(tdm, &mem_fmt, &channels);
	if (result) {
		dev_err(tdm->dev, "Failed to lookup mem fmt");
		return result;
	}

	set__AUD_UNIPERIF_CONFIG__MEM_FMT(tdm, mem_fmt);
	 
	set__AUD_UNIPERIF_CONFIG__FDMA_TRIGGER_LIMIT(tdm, tdm->dma_maxburst);

	set__AUD_UNIPERIF_I2S_FMT__NBIT_16(tdm);
	 
	set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_32(tdm);
	 
	set__AUD_UNIPERIF_I2S_FMT__LR_POL_HIG(tdm);
	 
	set__AUD_UNIPERIF_I2S_FMT__ALIGN_LEFT(tdm);
	 
	set__AUD_UNIPERIF_I2S_FMT__ORDER_MSB(tdm);
	 
	set__AUD_UNIPERIF_I2S_FMT__NUM_CH(tdm, channels);
	 
	set__AUD_UNIPERIF_I2S_FMT__PADDING_SONY_MODE(tdm);

	if (tdm->info->rising_edge)
		set__AUD_UNIPERIF_I2S_FMT__SCLK_EDGE_RISING(tdm);
	else
		set__AUD_UNIPERIF_I2S_FMT__SCLK_EDGE_FALLING(tdm);

	set__AUD_UNIPERIF_I2S_FMT__NO_OF_SAMPLES_TO_READ(tdm, 0);

	result = uniperif_tdm_set_freqs(tdm);
	if (result) {
		dev_err(tdm->dev, "Failed to set freqs");
		return result;
	}

	uniperif_tdm_configure_timeslots(tdm);

	return 0;
}

static int uniperif_tdm_clk_get(struct uniperif_tdm *tdm)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_TELSS);
	int result;

	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	if (tdm->info->fdma_direction == DMA_DEV_TO_MEM)
		return 0;

	tdm->clk = snd_stm_clk_get(tdm->dev, "uniperif_tdm_clk", card,
			tdm->info->card_device);
	if (IS_ERR(tdm->clk)) {
		dev_err(tdm->dev, "Failed to get clk");
		return -EINVAL;
	}

#ifdef MY_DEF_HERE
	result = snd_stm_clk_prepare_enable(tdm->clk);
#else  
	result = snd_stm_clk_enable(tdm->clk);
#endif  
	if (result) {
		dev_err(tdm->dev, "Failed to enable clk");
		goto error_clk_enable;
	}

	result = snd_stm_clk_set_rate(tdm->clk, tdm->info->clk_rate);
	if (result) {
		dev_err(tdm->dev, "Failed to set clk rate");
		goto error_clk_set_rate;
	}

	return 0;

error_clk_set_rate:
#ifdef MY_DEF_HERE
	snd_stm_clk_disable_unprepare(tdm->clk);
#else  
	snd_stm_clk_disable(tdm->clk);
#endif  
error_clk_enable:
	snd_stm_clk_put(tdm->clk);
	return result;
}

static void uniperif_tdm_clk_put(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	if (!IS_ERR(tdm->clk)) {
		 
#ifdef MY_DEF_HERE
		snd_stm_clk_disable_unprepare(tdm->clk);
#else  
		snd_stm_clk_disable(tdm->clk);
#endif  
		 
		snd_stm_clk_put(tdm->clk);
		 
		tdm->clk = NULL;
	}
}

static int uniperif_tdm_pclk_get(struct uniperif_tdm *tdm)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_TELSS);
	int result;

	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	if (tdm->info->fdma_direction == DMA_DEV_TO_MEM)
		return 0;

	tdm->pclk = snd_stm_clk_get(tdm->dev, "uniperif_tdm_pclk", card,
			tdm->info->card_device);
	if (IS_ERR(tdm->pclk)) {
		dev_err(tdm->dev, "Failed to get pclk");
		return -EINVAL;
	}

#ifdef MY_DEF_HERE
	result = snd_stm_clk_prepare_enable(tdm->pclk);
#else  
	result = snd_stm_clk_enable(tdm->pclk);
#endif  
	if (result) {
		dev_err(tdm->dev, "Failed to enable pclk");
		goto error_pclk_enable;
	}

	result = snd_stm_clk_set_rate(tdm->pclk, tdm->info->pclk_rate);
	if (result) {
		dev_err(tdm->dev, "Failed to set pclk rate");
		goto error_pclk_set_rate;
	}

	return 0;

error_pclk_set_rate:
#ifdef MY_DEF_HERE
	snd_stm_clk_disable_unprepare(tdm->pclk);
#else  
	snd_stm_clk_disable(tdm->pclk);
#endif  
error_pclk_enable:
	snd_stm_clk_put(tdm->pclk);
	return result;
}

static void uniperif_tdm_pclk_put(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	if (!IS_ERR(tdm->pclk)) {
		 
#ifdef MY_DEF_HERE
		snd_stm_clk_disable_unprepare(tdm->pclk);
#else  
		snd_stm_clk_disable(tdm->pclk);
#endif  
		 
		snd_stm_clk_put(tdm->pclk);
		 
		tdm->pclk = NULL;
	}
}

static void uniperif_tdm_hw_reset(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	set__AUD_UNIPERIF_SOFT_RST__SOFT_RST(tdm);

	while (get__AUD_UNIPERIF_SOFT_RST__SOFT_RST(tdm))
		udelay(5);
}

static void uniperif_tdm_hw_enable(struct uniperif_tdm *tdm)
{
	int i;

	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	set__AUD_UNIPERIF_TDM_ENABLE__TDM_ENABLE(tdm);

	if (tdm->info->fdma_direction == DMA_MEM_TO_DEV)
		for (i = 0; i < tdm->info->frame_size; ++i)
			set__AUD_UNIPERIF_FIFO_DATA(tdm, 0x00000000);

	set__AUD_UNIPERIF_CTRL__OPERATION_PCM_DATA(tdm);
}

static void uniperif_tdm_hw_disable(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	set__AUD_UNIPERIF_TDM_ENABLE__TDM_DISABLE(tdm);

	set__AUD_UNIPERIF_CTRL__OPERATION_OFF(tdm);

	uniperif_tdm_hw_reset(tdm);
}

static void uniperif_tdm_hw_start(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	tdm->dma_cookie = dmaengine_submit(tdm->dma_descriptor);

	tdm->dma_fifo_errors = 0;

	set__AUD_UNIPERIF_ITS_BCLR__FIFO_ERROR(tdm);
	set__AUD_UNIPERIF_ITM_BSET__FIFO_ERROR(tdm);
	set__AUD_UNIPERIF_ITS_BCLR__DMA_ERROR(tdm);
	set__AUD_UNIPERIF_ITM_BSET__DMA_ERROR(tdm);
	enable_irq(tdm->irq);

	if (tdm->info->fdma_direction == DMA_DEV_TO_MEM) {
		uniperif_tdm_hw_reset(tdm);
		uniperif_tdm_hw_enable(tdm);
	}
}

static void uniperif_tdm_hw_stop(struct uniperif_tdm *tdm)
{
	dev_dbg(tdm->dev, "%s(tdm=%p)", __func__, tdm);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	disable_irq_nosync(tdm->irq);
	set__AUD_UNIPERIF_ITM_BCLR__FIFO_ERROR(tdm);
	set__AUD_UNIPERIF_ITM_BCLR__DMA_ERROR(tdm);

	if (tdm->info->fdma_direction == DMA_DEV_TO_MEM)
		uniperif_tdm_hw_disable(tdm);

	dmaengine_terminate_all(tdm->dma_channel);
}

static void uniperif_tdm_dump_registers(struct snd_info_entry *entry,
		struct snd_info_buffer *buffer)
{
	struct uniperif_tdm *tdm = entry->private_data;

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	snd_iprintf(buffer, "--- %s (0x%p) ---\n", dev_name(tdm->dev),
			tdm->base);

	UNIPERIF_TDM_DUMP_REGISTER(SOFT_RST);
	UNIPERIF_TDM_DUMP_REGISTER(STA);
	UNIPERIF_TDM_DUMP_REGISTER(ITS);
	UNIPERIF_TDM_DUMP_REGISTER(ITM);
	UNIPERIF_TDM_DUMP_REGISTER(CONFIG);
	UNIPERIF_TDM_DUMP_REGISTER(CTRL);
	UNIPERIF_TDM_DUMP_REGISTER(I2S_FMT);
	UNIPERIF_TDM_DUMP_REGISTER(STATUS_1);
	UNIPERIF_TDM_DUMP_REGISTER(DFV0);
	UNIPERIF_TDM_DUMP_REGISTER(CONTROLABILITY);
	UNIPERIF_TDM_DUMP_REGISTER(CRC_CTRL);
	UNIPERIF_TDM_DUMP_REGISTER(CRC_WINDOW);
	UNIPERIF_TDM_DUMP_REGISTER(CRC_VALUE_IN);
	UNIPERIF_TDM_DUMP_REGISTER(CRC_VALUE_OUT);
	UNIPERIF_TDM_DUMP_REGISTER(TDM_ENABLE);

	if (tdm->info->fdma_direction == DMA_MEM_TO_DEV) {
		UNIPERIF_TDM_DUMP_REGISTER(TDM_FS_REF_FREQ);
		UNIPERIF_TDM_DUMP_REGISTER(TDM_FS_REF_DIV);
		UNIPERIF_TDM_DUMP_REGISTER(TDM_FS01_FREQ);
		UNIPERIF_TDM_DUMP_REGISTER(TDM_FS01_WIDTH);
		UNIPERIF_TDM_DUMP_REGISTER(TDM_FS02_FREQ);
		UNIPERIF_TDM_DUMP_REGISTER(TDM_FS02_WIDTH);
		UNIPERIF_TDM_DUMP_REGISTER(TDM_FS02_TIMESLOT_DELAY);
	}

	UNIPERIF_TDM_DUMP_REGISTER(TDM_DATA_MSBIT_START);
	UNIPERIF_TDM_DUMP_REGISTER(TDM_WORD_POS_1_2);
	UNIPERIF_TDM_DUMP_REGISTER(TDM_WORD_POS_3_4);
	UNIPERIF_TDM_DUMP_REGISTER(TDM_WORD_POS_5_6);
	UNIPERIF_TDM_DUMP_REGISTER(TDM_WORD_POS_7_8);

	snd_iprintf(buffer, "\n");
}

static int uniperif_tdm_register(struct snd_device *snd_device)
{
	struct uniperif_tdm *tdm = snd_device->device_data;
	int result;

	dev_dbg(tdm->dev, "%s(snd_device=%p)", __func__, snd_device);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	result = uniperif_tdm_clk_get(tdm);
	if (result) {
		dev_err(tdm->dev, "Failed to get clock");
		goto error_clk_get;
	}

	result = uniperif_tdm_pclk_get(tdm);
	if (result) {
		dev_err(tdm->dev, "Failed to get pclk");
		goto error_pclk_get;
	}

	result = uniperif_tdm_configure(tdm);
	if (result) {
		dev_err(tdm->dev, "Failed to configure");
		goto error_configure;
	}

	result = snd_stm_info_register(&tdm->proc_entry, dev_name(tdm->dev),
			uniperif_tdm_dump_registers, tdm);
	if (result) {
		dev_err(tdm->dev, "Failed to register with procfs");
		goto error_info_register;
	}

	if (tdm->info->fdma_direction == DMA_MEM_TO_DEV)
		uniperif_tdm_hw_enable(tdm);

	return 0;

error_info_register:
error_configure:
	uniperif_tdm_pclk_put(tdm);
error_pclk_get:
	uniperif_tdm_clk_put(tdm);
error_clk_get:
	return result;
}

static int uniperif_tdm_disconnect(struct snd_device *snd_device)
{
	struct uniperif_tdm *tdm = snd_device->device_data;

	dev_dbg(tdm->dev, "%s(snd_device=%p)", __func__, snd_device);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	uniperif_tdm_dma_release(tdm);

	uniperif_tdm_hw_disable(tdm);

	uniperif_tdm_pclk_put(tdm);
	uniperif_tdm_clk_put(tdm);

	snd_stm_info_unregister(tdm->proc_entry);

	return 0;
}

static struct snd_device_ops uniperif_tdm_snd_device_ops = {
	.dev_register	= uniperif_tdm_register,
	.dev_disconnect	= uniperif_tdm_disconnect,
};

static int uniperif_tdm_handset_init(struct uniperif_tdm *tdm, int id)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_TELSS);
	struct telss_handset *handset;
	int direction;
	int playback;
	int capture;
	char *name;
	int result;

	dev_dbg(tdm->dev, "%s(tdm=%p, id=%d)", __func__, tdm, id);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	handset = &tdm->handsets[id];

	handset->id = id;
	handset->ctl = uniperif_tdm_handset_pcm_ctl++;
	handset->tdm = tdm;
	handset->info = &tdm->info->handset_info[id];
	handset->period_max_sz = tdm->period_max_sz / tdm->info->handset_count;
	handset->buffer_max_sz = tdm->buffer_max_sz / tdm->info->handset_count;
	handset->buffer_act_sz = tdm->buffer_act_sz / tdm->info->handset_count;

	handset->buffer_offset = (handset->buffer_max_sz + PAGE_SIZE - 1);
	handset->buffer_offset /= PAGE_SIZE;
	handset->buffer_offset *= PAGE_SIZE * id;
	snd_stm_magic_set(handset);

	BUG_ON(handset->buffer_offset > UNIPERIF_TDM_BUF_OFF_MAX);

	switch (tdm->info->fdma_direction) {
	case DMA_MEM_TO_DEV:
		name = "TELSS Player #%d";
		capture = 0;
		playback = 1;
		direction = SNDRV_PCM_STREAM_PLAYBACK;
		break;

	case DMA_DEV_TO_MEM:
		name = "TELSS Reader #%d";
		capture = 1;
		playback = 0;
		direction = SNDRV_PCM_STREAM_CAPTURE;
		break;

	default:
		dev_err(tdm->dev, "Cannot determine if player/reader");
		return -EINVAL;
	}

	result = snd_pcm_new(card, NULL, handset->ctl, playback, capture,
				&handset->pcm);
	if (result) {
		dev_err(tdm->dev, "Failed to create pcm for handset %d", id);
		return result;
	}

	snprintf(handset->pcm->name, sizeof(handset->pcm->name), name, id);
	dev_notice(tdm->dev, "'%s'", handset->pcm->name);

	handset->pcm->private_data = handset;

	snd_pcm_set_ops(handset->pcm, direction, &uniperif_tdm_pcm_ops);

	return 0;
}

static void uniperif_tdm_parse_dt_handsets(struct platform_device *pdev,
		struct device_node *pnode,
		struct snd_stm_uniperif_tdm_info *info)
{
	struct device_node *hnode, *child;
	struct snd_stm_telss_handset_info *hs_info;
	int i = 0;

	BUG_ON(!pdev);
	BUG_ON(!pnode);
	BUG_ON(!info);

	of_property_read_u32(pnode, "handset-count", &info->handset_count);

	hnode = of_parse_phandle(pnode, "handset-info", 0);

	for_each_child_of_node(hnode, child)
		i++;

	BUG_ON(i != info->handset_count);

	hs_info = devm_kzalloc(&pdev->dev,
			sizeof(*hs_info) * info->handset_count,
			GFP_KERNEL);
	BUG_ON(!hs_info);
	info->handset_info = hs_info;

	for_each_child_of_node(hnode, child) {
		of_property_read_u32(child, "fsync", &hs_info->fsync);
		of_property_read_u32(child, "slot1", &hs_info->slot1);
		of_property_read_u32(child, "slot2", &hs_info->slot2);
		of_property_read_u32(child, "slot2-valid",
				&hs_info->slot2_valid);
		of_property_read_u32(child, "duplicate", &hs_info->duplicate);
		of_property_read_u32(child, "data16", &hs_info->data16);
		of_property_read_u32(child, "cnb", &hs_info->cnb);
		of_property_read_u32(child, "lnb", &hs_info->lnb);
		of_property_read_u32(child, "cwb", &hs_info->cwb);
		of_property_read_u32(child, "lwb", &hs_info->lwb);
		hs_info++;
	}
}

static void uniperif_tdm_parse_dt_hw_config(struct platform_device *pdev,
		struct snd_stm_uniperif_tdm_info *info)
{
	struct device_node *pnode;

	BUG_ON(!pdev);
	BUG_ON(!info);

	pnode = of_parse_phandle(pdev->dev.of_node, "hw-config", 0);

	of_property_read_u32(pnode, "pclk-rate", &info->pclk_rate);
	of_property_read_u32(pnode, "fs-rate", &info->fs_rate);
	of_property_read_u32(pnode, "fs-divider", &info->timeslots);
	of_property_read_u32(pnode, "fs01-rate", &info->fs01_rate);
	of_property_read_u32(pnode, "fs02-rate", &info->fs02_rate);
	of_property_read_u32(pnode, "fs02-delay-clock",
			&info->fs02_delay_clock);
	of_property_read_u32(pnode, "fs02-delay-timeslot",
			&info->fs02_delay_timeslot);
	of_property_read_u32(pnode, "msbit-start", &info->msbit_start);
	of_property_read_u32(pnode, "frame-size", &info->frame_size);

	uniperif_tdm_parse_dt_handsets(pdev, pnode, info);
}

static int uniperif_tdm_parse_dt(struct platform_device *pdev,
		struct uniperif_tdm *tdm)
{
	struct snd_stm_uniperif_tdm_info *info;
	struct device_node *pnode;
	const char *direction;
	int val;

	BUG_ON(!pdev);
	BUG_ON(!tdm);

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	BUG_ON(!info);

	pnode = pdev->dev.of_node;

	of_property_read_string(pnode, "description", &info->name);
	of_property_read_u32(pnode, "version", &info->ver);
	of_property_read_u32(pnode, "card-device", &info->card_device);

	val = get_property_hdl(&pdev->dev, pnode, "dmas", 0);
	if (!val) {
		dev_err(&pdev->dev, "unable to find DT dma node");
		return -EINVAL;
	}
	info->dma_np = of_find_node_by_phandle(val);
	of_property_read_string(pnode, "fdma-name", &info->fdma_name);
	of_property_read_u32(pnode, "fdma-channel", &info->fdma_channel);
	of_property_read_u32(pnode, "fdma-initiator", &info->fdma_initiator);
	of_property_read_string(pnode, "fdma-direction", &direction);
	of_property_read_u32(pnode, "fdma-direct-conn",
			&info->fdma_direct_conn);
	of_property_read_u32(pnode, "fdma-request-line",
			&info->fdma_request_line);

	if (strcasecmp(direction, "DMA_MEM_TO_DEV") == 0)
		info->fdma_direction = DMA_MEM_TO_DEV;
	else if (strcasecmp(direction, "DMA_DEV_TO_MEM") == 0)
		info->fdma_direction = DMA_DEV_TO_MEM;
	else
		info->fdma_direction = DMA_TRANS_NONE;

	of_property_read_u32(pnode, "rising-edge", &info->rising_edge);
	of_property_read_u32(pnode, "clk-rate", &info->clk_rate);
	of_property_read_u32(pnode, "frame-count", &info->frame_count);
	of_property_read_u32(pnode, "max-periods", &info->max_periods);

	uniperif_tdm_parse_dt_hw_config(pdev, info);

	tdm->info = info;

	return 0;
}

static int uniperif_tdm_probe(struct platform_device *pdev)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_TELSS);
	struct uniperif_tdm *tdm;
	int result;
	int pages;
	int h;

	BUG_ON(!pdev);
	BUG_ON(!card);

	dev_dbg(&pdev->dev, "%s(pdev=%p)\n", __func__, pdev);

	tdm = devm_kzalloc(&pdev->dev, sizeof(*tdm), GFP_KERNEL);
	if (!tdm) {
		dev_err(&pdev->dev, "Failed to allocate device structure");
		return -ENOMEM;
	}

	tdm->dev = &pdev->dev;
	snd_stm_magic_set(tdm);

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "device not in dt");
		return -EINVAL;
	}

	if (uniperif_tdm_parse_dt(pdev, tdm)) {
		dev_err(&pdev->dev, "invalid dt");
		return -EINVAL;
	}

	tdm->ver = tdm->info->ver;
	BUG_ON(tdm->ver < 0);

	spin_lock_init(&tdm->lock);
	mutex_init(&tdm->dma_mutex);

	dev_notice(&pdev->dev, "'%s'", tdm->info->name);

	result = snd_stm_memory_request(pdev, &tdm->mem_region, &tdm->base);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed memory request");
		return result;
	}

	result = snd_stm_irq_request(pdev, &tdm->irq, uniperif_tdm_irq_handler,
			tdm);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed IRQ request");
		return result;
	}

	result = snd_device_new(card, SNDRV_DEV_LOWLEVEL, tdm,
			&uniperif_tdm_snd_device_ops);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed to create ALSA sound device");
		goto error_device_new;
	}

	BUG_ON(tdm->info->handset_count < UNIPERIF_TDM_MIN_HANDSETS);
	BUG_ON(tdm->info->handset_count > UNIPERIF_TDM_MAX_HANDSETS);

	BUG_ON(tdm->info->frame_count % 8);

	tdm->period_max_sz = tdm->info->frame_size * 4 * tdm->info->frame_count;

	tdm->period_act_cnt = UNIPERIF_TDM_BUF_OFF_MAX;
	tdm->period_act_cnt /= tdm->period_max_sz;
	tdm->period_act_cnt /= tdm->info->handset_count;

	BUG_ON(tdm->period_act_cnt < UNIPERIF_TDM_MIN_PERIODS);

	if (tdm->info->max_periods < UNIPERIF_TDM_MIN_PERIODS)
		tdm->info->max_periods = UNIPERIF_TDM_DEF_PERIODS;

	if (tdm->period_act_cnt > tdm->info->max_periods)
		tdm->period_act_cnt = tdm->info->max_periods;

	if (tdm->period_act_cnt != tdm->info->max_periods)
		dev_notice(&pdev->dev, "Using %d periods not %d",
				tdm->period_act_cnt, tdm->info->max_periods);

	tdm->period_max_cnt = tdm->period_act_cnt;

	tdm->buffer_max_sz = tdm->period_max_sz * tdm->period_act_cnt;
	tdm->buffer_act_sz = tdm->buffer_max_sz;

	tdm->dma_maxburst = UNIPERIF_TDM_DMA_MAXBURST / tdm->info->frame_size;
	tdm->dma_maxburst *= tdm->info->frame_size;

	tdm->buffer_bpa2 = bpa2_find_part(CONFIG_SND_STM_BPA2_PARTITION_NAME);
	if (!tdm->buffer_bpa2) {
		dev_err(&pdev->dev, "Failed to find BPA2 partition");
		result = -ENOMEM;
		goto error_bpa2_find;
	}
	dev_notice(&pdev->dev, "BPA2 '%s' at %p",
			CONFIG_SND_STM_BPA2_PARTITION_NAME, tdm->buffer_bpa2);

	pages = (tdm->buffer_max_sz / tdm->info->handset_count) + PAGE_SIZE - 1;
	pages /= PAGE_SIZE;
	pages *= tdm->info->handset_count;

	tdm->buffer_phys = bpa2_alloc_pages(tdm->buffer_bpa2, pages,
				0, GFP_KERNEL);
	if (!tdm->buffer_phys) {
		dev_err(tdm->dev, "Failed to allocate BPA2 pages");
		result = -ENOMEM;
		goto error_bpa2_alloc;
	}

	if (pfn_valid(__phys_to_pfn(tdm->buffer_phys))) {
		dev_err(tdm->dev,
			"BPA2 page 0x%08x is inside the kernel address space",
			tdm->buffer_phys);
		result = -ENOMEM;
		goto error_bpa2_alloc;
	}

	tdm->buffer_area = ioremap_nocache(tdm->buffer_phys, pages * PAGE_SIZE);
	BUG_ON(!tdm->buffer_area);

	dev_notice(tdm->dev, "Buffer at %08x (phys)", tdm->buffer_phys);
	dev_notice(tdm->dev, "Buffer at %p (virt)", tdm->buffer_area);

	for (h = 0; h < tdm->info->handset_count; ++h) {
		 
		result = uniperif_tdm_handset_init(tdm, h);
		if (result) {
			dev_err(&pdev->dev, "Failed to init handset %d", h);
			goto error_handset_init;
		}
	}

	result = snd_card_register(card);
	if (result) {
		dev_err(&pdev->dev, "Failed to register card (%d)", result);
		goto error_card_register;
	}

	platform_set_drvdata(pdev, tdm);

	return 0;

error_card_register:
error_handset_init:
	iounmap(tdm->buffer_area);
	bpa2_free_pages(tdm->buffer_bpa2, tdm->buffer_phys);
error_bpa2_alloc:
error_bpa2_find:
	snd_device_free(card, tdm);
error_device_new:
	 
	return result;
}

static int uniperif_tdm_remove(struct platform_device *pdev)
{
	struct uniperif_tdm *tdm = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s(pdev=%p)\n", __func__, pdev);

	BUG_ON(!tdm);
	BUG_ON(!snd_stm_magic_valid(tdm));

	iounmap(tdm->buffer_area);
	bpa2_free_pages(tdm->buffer_bpa2, tdm->buffer_phys);

	snd_stm_magic_clear(tdm);

	return 0;
}

#ifdef CONFIG_PM
static int uniperif_tdm_suspend(struct device *dev)
{
	dev_dbg(dev, "%s(dev=%p)", __func__, dev);
	dev_err(dev, "PM not supported!");
	return -ENOTSUPP;
}

static int uniperif_tdm_resume(struct device *dev)
{
	dev_dbg(dev, "%s(dev=%p)", __func__, dev);
	dev_err(dev, "PM not supported!");
	return -ENOTSUPP;
}

static const struct dev_pm_ops uniperif_tdm_pm_ops = {
	.suspend = uniperif_tdm_suspend,
	.resume	 = uniperif_tdm_resume,

	.freeze	 = uniperif_tdm_suspend,
	.thaw	 = uniperif_tdm_resume,
	.restore = uniperif_tdm_resume,
};
#endif

static struct of_device_id uniperif_tdm_match[] = {
	{
		.compatible = "st,snd_uni_tdm",
	},
	{},
};

MODULE_DEVICE_TABLE(of, uniperif_tdm_match);

static struct platform_driver uniperif_tdm_platform_driver = {
	.driver.name	= "snd_uni_tdm",
	.driver.of_match_table = uniperif_tdm_match,
#ifdef CONFIG_PM
	.driver.pm	= &uniperif_tdm_pm_ops,
#endif
	.probe		= uniperif_tdm_probe,
	.remove		= uniperif_tdm_remove,
};

module_platform_driver(uniperif_tdm_platform_driver);

MODULE_AUTHOR("John Boddie <john.boddie@st.com>");
MODULE_DESCRIPTION("STMicroelectronics Uniperipheral TDM driver");
MODULE_LICENSE("GPL");
