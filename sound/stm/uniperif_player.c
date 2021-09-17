#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <asm/cacheflush.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/control.h>
#include <sound/info.h>
#include <sound/pcm_params.h>
#include <linux/platform_data/dma-st-fdma.h>
#include "common.h"
#include "reg_aud_uniperif.h"
#include <linux/pinctrl/consumer.h>

#define DEFAULT_FORMAT (SND_STM_FORMAT__I2S | SND_STM_FORMAT__SUBFRAME_32_BITS)
#define DEFAULT_OVERSAMPLING 128  

#define MAX_SAMPLES_PER_PERIOD ((1 << 20) - 1)

#define MIN_IEC958_SAMPLE_RATE	32000

#ifdef MY_DEF_HERE
#define PARKING_SUBBLOCKS	2
#else  
#define PARKING_SUBBLOCKS	4
#define PARKING_NBFRAMES	4
#endif  
#define PARKING_BUFFER_SIZE	128	 

#define UNIPERIF_FIFO_SIZE	70	 
#define UNIPERIF_FIFO_FRAMES	4	 
#define UNIPERIF_PLAYER_UNDERFLOW_US	1000

#define MIN_AUTOSUPEND_DELAY_MS	100
#define UNIPERIF_PLAYER_NO_SUSPEND	(-1)
 
enum uniperif_iec958_input_mode {
	UNIPERIF_IEC958_INPUT_MODE_PCM,
	UNIPERIF_IEC958_INPUT_MODE_RAW
};

enum uniperif_iec958_encoding_mode {
	UNIPERIF_IEC958_ENCODING_MODE_PCM,
	UNIPERIF_IEC958_ENCODING_MODE_ENCODED
};

struct uniperif_iec958_settings {
	enum uniperif_iec958_input_mode input_mode;
	enum uniperif_iec958_encoding_mode encoding_mode;
	struct snd_aes_iec958 iec958;
};

enum uniperif_player_state {
	UNIPERIF_PLAYER_STATE_STOPPED,
	UNIPERIF_PLAYER_STATE_STARTED,
	UNIPERIF_PLAYER_STATE_PARKING,
	UNIPERIF_PLAYER_STATE_STANDBY,
	UNIPERIF_PLAYER_STATE_UNDERFLOW
};

enum snd_stm_uniperif_player_type {
	SND_STM_UNIPERIF_PLAYER_TYPE_NONE,
	SND_STM_UNIPERIF_PLAYER_TYPE_HDMI,
	SND_STM_UNIPERIF_PLAYER_TYPE_PCM,
	SND_STM_UNIPERIF_PLAYER_TYPE_SPDIF
};

struct snd_stm_uniperif_player_info {
	const char *name;
	int ver;

	int card_device;
	enum snd_stm_uniperif_player_type player_type;

	unsigned int iec958_lr_pol;		 
	unsigned int iec958_i2s_mode;		 

	unsigned int channels;

	int s16_swap_lr;			 
	int parking_enabled;
	int standby_enabled;
	int underflow_enabled;			 

	const char *fdma_name;
	struct device_node *dma_np;
	unsigned int fdma_initiator;
	unsigned int fdma_direct_conn;
	unsigned int fdma_request_line;

	unsigned int suspend_delay;
};

struct uniperif_player {
	 
	struct snd_stm_uniperif_player_info *info;
	struct device *dev;
	struct snd_pcm *pcm;
	int ver;  

	struct resource *mem_region;
	void *base;
	unsigned long fifo_phys_address;
	unsigned int irq;
	int fdma_channel;
#ifdef MY_DEF_HERE
	spinlock_t lock;
#endif  

	struct snd_stm_clk *clock;
	struct snd_pcm_hw_constraint_list channels_constraint;
	struct snd_stm_conv_source *conv_source;

	enum uniperif_player_state state;
	struct snd_stm_conv_group *conv_group;
	struct snd_stm_buffer *buffer;
	struct snd_info_entry *proc_entry;
	struct snd_pcm_substream *substream;

	struct snd_pcm_hardware hardware;

	spinlock_t default_settings_lock;
	struct uniperif_iec958_settings default_settings;
	struct uniperif_iec958_settings stream_settings;

	int dma_max_transfer_size;
	struct st_dma_audio_config dma_config;
	struct dma_chan *dma_channel;
	struct dma_async_tx_descriptor *dma_descriptor;
	dma_cookie_t dma_cookie;
	struct st_dma_park_config dma_park_config;

	int underflow_us;

	int buffer_bytes;
	int period_bytes;

	unsigned int format;
	unsigned int oversampling;

	unsigned int current_rate;
	unsigned int current_format;
	unsigned int current_channels;

	const char *clk_name;

	snd_stm_magic_field;
};

#define UNIPERIF_PLAYER_TYPE_IS_HDMI(p) \
	((p)->info->player_type == SND_STM_UNIPERIF_PLAYER_TYPE_HDMI)
#define UNIPERIF_PLAYER_TYPE_IS_PCM(p) \
	((p)->info->player_type == SND_STM_UNIPERIF_PLAYER_TYPE_PCM)
#define UNIPERIF_PLAYER_TYPE_IS_SPDIF(p) \
	((p)->info->player_type == SND_STM_UNIPERIF_PLAYER_TYPE_SPDIF)
#define UNIPERIF_PLAYER_TYPE_IS_IEC958(p) \
	(UNIPERIF_PLAYER_TYPE_IS_HDMI(p) || \
		UNIPERIF_PLAYER_TYPE_IS_SPDIF(p))

static struct snd_pcm_hardware uniperif_player_pcm_hw = {
	.info		= (SNDRV_PCM_INFO_MMAP |
				SNDRV_PCM_INFO_MMAP_VALID |
				SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER |
				SNDRV_PCM_INFO_PAUSE),
	.formats	= (SNDRV_PCM_FMTBIT_S32_LE |
				SNDRV_PCM_FMTBIT_S16_LE),

	.rates		= SNDRV_PCM_RATE_CONTINUOUS,
	.rate_min	= 8000,
	.rate_max	= 192000,

	.channels_min	= 2,
	.channels_max	= 10,

	.periods_min	= 2,
	.periods_max	= 1024,   

	.period_bytes_min = 1280,  
	.period_bytes_max = 196608,  
	.buffer_bytes_max = 196608 * 3,  
};

static struct snd_pcm_hardware uniperif_player_raw_hw = {
	.info		= (SNDRV_PCM_INFO_MMAP |
				SNDRV_PCM_INFO_MMAP_VALID |
				SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER |
				SNDRV_PCM_INFO_PAUSE),
	.formats	= (SNDRV_PCM_FMTBIT_S32_LE),

	.rates		= SNDRV_PCM_RATE_CONTINUOUS,
	.rate_min	= 32000,
	.rate_max	= 192000,

	.channels_min	= 2,
	.channels_max	= 2,

	.periods_min	= 2,
	.periods_max	= 1024,   

	.period_bytes_min = 1280,  
	.period_bytes_max = 81920,  
	.buffer_bytes_max = 81920 * 3,  
};

static int uniperif_player_stop(struct snd_pcm_substream *substream);

static irqreturn_t uniperif_player_irq_handler(int irq, void *dev_id)
{
	irqreturn_t result = IRQ_NONE;
	struct uniperif_player *player = dev_id;
	unsigned int status;
	unsigned int tmp;

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	preempt_disable();
	status = get__AUD_UNIPERIF_ITS(player);
	set__AUD_UNIPERIF_ITS_BCLR(player, status);
	preempt_enable();

	snd_pcm_stream_lock(player->substream);
	if ((player->state == UNIPERIF_PLAYER_STATE_STANDBY) ||
		(player->state == UNIPERIF_PLAYER_STATE_STOPPED)) {
		 
		dev_warn(player->dev, "unexpected IRQ: status flag: %#x",
			status);
		snd_pcm_stream_unlock(player->substream);

		return IRQ_HANDLED;
	};
	 
	if (unlikely(status & mask__AUD_UNIPERIF_ITS__FIFO_ERROR(player))) {
		dev_err(player->dev, "FIFO error detected");

		if (player->info->underflow_enabled) {
			 
			player->state = UNIPERIF_PLAYER_STATE_UNDERFLOW;
		} else {
			 
			set__AUD_UNIPERIF_ITM_BCLR__FIFO_ERROR(player);

			snd_pcm_stop(player->substream, SNDRV_PCM_STATE_XRUN);
		}

		result = IRQ_HANDLED;
	}

	if (unlikely(status & mask__AUD_UNIPERIF_ITS__DMA_ERROR(player))) {
		dev_err(player->dev, "DMA error detected");

		set__AUD_UNIPERIF_ITM_BCLR__DMA_ERROR(player);

		snd_pcm_stop(player->substream, SNDRV_PCM_STATE_XRUN);

		result = IRQ_HANDLED;
	}

	if (unlikely(status &
			mask__AUD_UNIPERIF_ITM__UNDERFLOW_REC_DONE(player))) {
		BUG_ON(!player->info->underflow_enabled);

		tmp = get__AUD_UNIPERIF_STATUS_1__UNDERFLOW_DURATION(player);
		dev_dbg(player->dev, "Underflow recovered (%d LR clocks)", tmp);

		set__AUD_UNIPERIF_BIT_CONTROL__CLR_UNDERFLOW_DURATION(player);

		if (player->state == UNIPERIF_PLAYER_STATE_UNDERFLOW)
			player->state = UNIPERIF_PLAYER_STATE_STARTED;

		result = IRQ_HANDLED;
	}

	if (unlikely(status &
			mask__AUD_UNIPERIF_ITM__UNDERFLOW_REC_FAILED(player))) {
		BUG_ON(!player->info->underflow_enabled);

		dev_err(player->dev, "Underflow recovery failed");

		snd_pcm_stop(player->substream, SNDRV_PCM_STATE_XRUN);

		result = IRQ_HANDLED;
	}

	BUG_ON(result != IRQ_HANDLED);

	snd_pcm_stream_unlock(player->substream);
	return result;
}

static bool uniperif_player_dma_filter_fn(struct dma_chan *chan, void *fn_param)
{
	struct uniperif_player *player = fn_param;
	struct st_dma_audio_config *config = &player->dma_config;

	BUG_ON(!player);

	if (player->info->dma_np)
		if (player->info->dma_np != chan->device->dev->of_node)
			return false;

	config->type = ST_DMA_TYPE_AUDIO;
	config->dma_addr = player->fifo_phys_address;
	config->dreq_config.request_line = player->info->fdma_request_line;
	config->dreq_config.direct_conn = player->info->fdma_direct_conn;
	config->dreq_config.initiator = player->info->fdma_initiator;
	config->dreq_config.increment = 0;
	config->dreq_config.hold_off = 0;
	config->dreq_config.maxburst = 1;
	config->dreq_config.buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
	config->dreq_config.direction = DMA_MEM_TO_DEV;

	config->park_config.sub_periods = PARKING_SUBBLOCKS;
	config->park_config.buffer_size = PARKING_BUFFER_SIZE;

	chan->private = config;

	dev_dbg(player->dev, "Using FDMA '%s' channel %d",
			dev_name(chan->device->dev), chan->chan_id);
	return true;
}

static int uniperif_player_resources_request(struct uniperif_player *player)
{
	dma_cap_mask_t mask;

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	if (!player->dma_channel) {
		 
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		dma_cap_set(DMA_CYCLIC, mask);

		player->dma_channel = dma_request_channel(mask,
				uniperif_player_dma_filter_fn, player);
		if (!player->dma_channel) {
			dev_err(player->dev, "Failed to request DMA channel");
			return -ENODEV;
		}
	}

	if (!player->conv_group) {
		 
		player->conv_group =
			snd_stm_conv_request_group(player->conv_source);

		dev_dbg(player->dev, "Attached to converter '%s'",
			player->conv_group ?
			snd_stm_conv_get_name(player->conv_group) : "*NONE*");
	}

	return 0;
}

static void uniperif_player_resources_release(struct uniperif_player *player)
{
	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	if (player->state == UNIPERIF_PLAYER_STATE_PARKING)
		return;

	if (player->state == UNIPERIF_PLAYER_STATE_STANDBY)
		return;

	BUG_ON(player->state != UNIPERIF_PLAYER_STATE_STOPPED);

	if (player->dma_channel) {
		 
		dmaengine_terminate_all(player->dma_channel);

		dma_release_channel(player->dma_channel);
		player->dma_channel = NULL;
	}

	if (player->conv_group) {
		 
		snd_stm_conv_release_group(player->conv_group);
		player->conv_group = NULL;
	}
}

static int uniperif_player_open(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int result;
#ifdef MY_DEF_HERE
	unsigned long irqflags;
#endif  

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!runtime);

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STARTED);
	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_UNDERFLOW);

	snd_pcm_set_sync(substream);   
#ifdef MY_DEF_HERE
	pm_runtime_get_sync(player->dev);
#else  
	pm_runtime_get(player->dev);
#endif  

	result = uniperif_player_resources_request(player);
	if (result) {
		dev_err(player->dev, "Failed to request resources");
		return result;
	}

	spin_lock(&player->default_settings_lock);
	player->stream_settings = player->default_settings;
	spin_unlock(&player->default_settings_lock);

	result = snd_pcm_hw_constraint_list(runtime, 0,
				SNDRV_PCM_HW_PARAM_CHANNELS,
				&player->channels_constraint);
	if (result < 0) {
		dev_err(player->dev, "Failed to set channel constraint");
		goto error;
	}

	result = snd_pcm_hw_constraint_integer(runtime,
			SNDRV_PCM_HW_PARAM_PERIODS);
	if (result < 0) {
		dev_err(player->dev, "Failed to constrain buffer periods");
		goto error;
	}

#ifdef MY_DEF_HERE
#else  
	 
	result = snd_stm_pcm_hw_constraint_transfer_bytes(runtime,
			UNIPERIF_FIFO_SIZE * 4);
	if (result < 0) {
		dev_err(player->dev, "Failed to constrain buffer bytes");
		goto error;
	}
#endif  

	runtime->hw = player->hardware;

	player->substream = substream;

#ifdef MY_DEF_HERE
	 
	spin_lock_irqsave(&player->lock, irqflags);
	result = snd_stm_clk_prepare(player->clock);
	if (result) {
		spin_unlock_irqrestore(&player->lock, irqflags);
		dev_err(player->dev, "Failed to prepare clock");
		return result;
	}
	spin_unlock_irqrestore(&player->lock, irqflags);
#endif  

	return 0;

error:
	 
	uniperif_player_resources_release(player);
	return result;
}

static int uniperif_player_close(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
#ifdef MY_DEF_HERE
	int result;
	unsigned long irqflags;
#endif  

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STARTED);
	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_UNDERFLOW);

	if (player->state != UNIPERIF_PLAYER_STATE_STOPPED) {
		if (player->info->suspend_delay !=
				UNIPERIF_PLAYER_NO_SUSPEND) {
			pm_runtime_mark_last_busy(player->dev);
#ifdef MY_DEF_HERE
			pm_runtime_put_sync_autosuspend(player->dev);
#else  
			pm_runtime_put_autosuspend(player->dev);
#endif  
		}
		return 0;
	}

	uniperif_player_resources_release(player);

	player->substream = NULL;

#ifdef MY_DEF_HERE
	 
	spin_lock_irqsave(&player->lock, irqflags);
	result = snd_stm_clk_unprepare(player->clock);
	if (result) {
		spin_unlock_irqrestore(&player->lock, irqflags);
		dev_err(player->dev, "Failed to unprepare clock");
		return result;
	}
	spin_unlock_irqrestore(&player->lock, irqflags);
#endif  

	return 0;
}

static int uniperif_player_hw_free(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!runtime);

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STARTED);
	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_UNDERFLOW);

	if (snd_stm_buffer_is_allocated(player->buffer)) {
		 
		if ((player->state != UNIPERIF_PLAYER_STATE_PARKING) &&
							(player->dma_channel))
			dmaengine_terminate_all(player->dma_channel);

		snd_stm_buffer_free(player->buffer);
	}

	return 0;
}

static void uniperif_player_comp_cb(void *param)
{
	struct uniperif_player *player = param;

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!player->substream);

	snd_pcm_period_elapsed(player->substream);
}

static int uniperif_player_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int buffer_bytes, frame_bytes, period_bytes, periods;
	int transfer_size, transfer_bytes, trigger_limit;
	struct dma_slave_config slave_config;
	int result;

	dev_dbg(player->dev, "%s(substream=%p, hw_params=%p)", __func__,
			substream, hw_params);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!runtime);

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STARTED);
	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_UNDERFLOW);

	if (snd_stm_buffer_is_allocated(player->buffer))
		uniperif_player_hw_free(substream);

	buffer_bytes = params_buffer_bytes(hw_params);
	periods = params_periods(hw_params);
	period_bytes = buffer_bytes / periods;
	BUG_ON(periods * period_bytes != buffer_bytes);

	result = snd_stm_buffer_alloc(player->buffer, substream, buffer_bytes);
	if (result) {
		dev_err(player->dev, "Failed to allocate %d-byte buffer",
				buffer_bytes);
		result = -ENOMEM;
		goto error_buf_alloc;
	}

	transfer_size = params_channels(hw_params) * UNIPERIF_FIFO_FRAMES;
	transfer_bytes = transfer_size * 4;

	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0)
		trigger_limit = UNIPERIF_FIFO_SIZE - transfer_size;
	else
		 
		trigger_limit = transfer_size;

	BUG_ON(trigger_limit % 2);

#ifdef MY_DEF_HERE
#else  
	 
	BUG_ON(period_bytes % transfer_bytes);
	BUG_ON(buffer_bytes % transfer_bytes);
#endif  

	BUG_ON(trigger_limit != 1 && transfer_size % 2);
	BUG_ON(trigger_limit >
			mask__AUD_UNIPERIF_CONFIG__FDMA_TRIGGER_LIMIT(player));

	dev_dbg(player->dev, "FDMA trigger limit %d", trigger_limit);

	set__AUD_UNIPERIF_CONFIG__FDMA_TRIGGER_LIMIT(player, trigger_limit);

	player->buffer_bytes = buffer_bytes;
	player->period_bytes = period_bytes;

	result = uniperif_player_resources_request(player);
	if (result) {
		dev_err(player->dev, "Failed to request resources");
		return result;
	}

	slave_config.direction = DMA_MEM_TO_DEV;
	slave_config.dst_addr = player->fifo_phys_address;
	slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	slave_config.dst_maxburst = transfer_size;

	result = dmaengine_slave_config(player->dma_channel, &slave_config);
	if (result) {
		dev_err(player->dev, "Failed to configure DMA channel");
		goto error_dma_config;
	}

	frame_bytes = snd_pcm_format_physical_width(params_format(hw_params)) *
			params_channels(hw_params) / 8;

	player->dma_park_config.sub_periods = PARKING_SUBBLOCKS;
	player->dma_park_config.buffer_size = transfer_bytes + (frame_bytes-1);
	player->dma_park_config.buffer_size /= frame_bytes;
	player->dma_park_config.buffer_size *= frame_bytes;

	return 0;

error_dma_config:
	snd_stm_buffer_free(player->buffer);
error_buf_alloc:
	return result;
}

static void uniperif_player_set_underflow(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	unsigned int window;

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!substream->runtime);

	if (!player->info->underflow_enabled) {
		 
		set__AUD_UNIPERIF_CTRL__UNDERFLOW_REC_WINDOW(player, 0);
		return;
	}

	window = player->underflow_us * (substream->runtime->rate / 1000);
	 
	window += 500;
	 
	window /= 1000;

	if (window > mask__AUD_UNIPERIF_CTRL__UNDERFLOW_REC_WINDOW(player))
		window = mask__AUD_UNIPERIF_CTRL__UNDERFLOW_REC_WINDOW(player);

	set__AUD_UNIPERIF_CTRL__UNDERFLOW_REC_WINDOW(player, window);
}

static int uniperif_player_prepare_pcm(struct uniperif_player *player,
		struct snd_pcm_runtime *runtime)
{
	int bits_in_output_frame;
	int lr_pol;

	dev_dbg(player->dev, "%s(player=%p, runtime=%p)", __func__,
			player, runtime);

	BUG_ON(player->state != UNIPERIF_PLAYER_STATE_STOPPED);

	player->format = DEFAULT_FORMAT;
	player->oversampling = DEFAULT_OVERSAMPLING;

	if (player->conv_group) {
		player->format = snd_stm_conv_get_format(player->conv_group);
		player->oversampling =
			snd_stm_conv_get_oversampling(player->conv_group);
		if (player->oversampling == 0)
			player->oversampling = DEFAULT_OVERSAMPLING;
	}

	dev_dbg(player->dev, "Sample frequency %d (oversampling %d)",
			runtime->rate, player->oversampling);

	BUG_ON(player->oversampling < 0);

	BUG_ON((player->format & SND_STM_FORMAT__SUBFRAME_32_BITS) &&
			(player->oversampling % 128));
	BUG_ON((player->format & SND_STM_FORMAT__SUBFRAME_16_BITS) &&
			(player->oversampling % 64));

	switch (player->format & SND_STM_FORMAT__SUBFRAME_MASK) {
	case SND_STM_FORMAT__SUBFRAME_32_BITS:
		dev_dbg(player->dev, "32-bit subframe");
		set__AUD_UNIPERIF_I2S_FMT__NBIT_32(player);
		set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_32(player);
		bits_in_output_frame = 64;  
		break;
	case SND_STM_FORMAT__SUBFRAME_16_BITS:
		dev_dbg(player->dev, "16-bit subframe");
		set__AUD_UNIPERIF_I2S_FMT__NBIT_16(player);
		set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_16(player);
		bits_in_output_frame = 32;  
		break;
	default:
		snd_BUG();
		return -EINVAL;
	}

	switch (player->format & SND_STM_FORMAT__MASK) {
	case SND_STM_FORMAT__I2S:
		dev_dbg(player->dev, "I2S format");
		set__AUD_UNIPERIF_I2S_FMT__ALIGN_LEFT(player);
		set__AUD_UNIPERIF_I2S_FMT__PADDING_I2S_MODE(player);
		lr_pol = value__AUD_UNIPERIF_I2S_FMT__LR_POL_LOW(player);
		break;
	case SND_STM_FORMAT__LEFT_JUSTIFIED:
		dev_dbg(player->dev, "Left Justified format");
		set__AUD_UNIPERIF_I2S_FMT__ALIGN_LEFT(player);
		set__AUD_UNIPERIF_I2S_FMT__PADDING_SONY_MODE(player);
		lr_pol = value__AUD_UNIPERIF_I2S_FMT__LR_POL_HIG(player);
		break;
	case SND_STM_FORMAT__RIGHT_JUSTIFIED:
		dev_dbg(player->dev, "Right Justified format");
		set__AUD_UNIPERIF_I2S_FMT__ALIGN_RIGHT(player);
		set__AUD_UNIPERIF_I2S_FMT__PADDING_SONY_MODE(player);
		lr_pol = value__AUD_UNIPERIF_I2S_FMT__LR_POL_HIG(player);
		break;
	default:
		snd_BUG();
		return -EINVAL;
	}

	switch (runtime->format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		 
		set__AUD_UNIPERIF_CONFIG__MEM_FMT_16_16(player);
		break;

	case SNDRV_PCM_FORMAT_S32_LE:
		 
		set__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(player);
		break;

	default:
		snd_BUG();
		return -EINVAL;
	}

	set__AUD_UNIPERIF_I2S_FMT__LR_POL(player, lr_pol);

	set__AUD_UNIPERIF_CTRL__ROUNDING_OFF(player);

	set__AUD_UNIPERIF_CTRL__DIVIDER(player,
			player->oversampling / (2 * bits_in_output_frame));

	BUG_ON(runtime->channels % 2);
	BUG_ON(runtime->channels < 2);
	BUG_ON(runtime->channels > 10);

	set__AUD_UNIPERIF_I2S_FMT__NUM_CH(player, runtime->channels / 2);

	set__AUD_UNIPERIF_CONFIG__ONE_BIT_AUD_DISABLE(player);

	set__AUD_UNIPERIF_I2S_FMT__ORDER_MSB(player);
	set__AUD_UNIPERIF_I2S_FMT__SCLK_EDGE_FALLING(player);

	set__AUD_UNIPERIF_CTRL__SPDIF_FMT_OFF(player);

	uniperif_player_set_underflow(player->substream);

	return 0;
}

static void uniperif_player_set_channel_status(struct uniperif_player *player,
		struct snd_pcm_runtime *runtime)
{
	int n;
	unsigned int status;

	dev_dbg(player->dev, "%s(player=%p, runtime=%p)", __func__,
			player, runtime);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	if (runtime && (player->stream_settings.iec958.status[3] == 0x01)) {
		switch (runtime->rate) {
		case 22050:
			player->stream_settings.iec958.status[3] = 0x04;
			break;
		case 44100:
			player->stream_settings.iec958.status[3] = 0x00;
			break;
		case 88200:
			player->stream_settings.iec958.status[3] = 0x08;
			break;
		case 176400:
			player->stream_settings.iec958.status[3] = 0x0c;
			break;
		case 24000:
			player->stream_settings.iec958.status[3] = 0x06;
			break;
		case 48000:
			player->stream_settings.iec958.status[3] = 0x02;
			break;
		case 96000:
			player->stream_settings.iec958.status[3] = 0x0a;
			break;
		case 192000:
			player->stream_settings.iec958.status[3] = 0x0e;
			break;
		case 32000:
			player->stream_settings.iec958.status[3] = 0x03;
			break;
		case 768000:
			player->stream_settings.iec958.status[3] = 0x09;
			break;
		default:
			 
			player->stream_settings.iec958.status[3] = 0x01;
			break;
		}
	}

	for (n = 0; n < 6; ++n) {
		status  = player->stream_settings.iec958.status[0+(n*4)] & 0xf;
		status |= player->stream_settings.iec958.status[1+(n*4)] << 8;
		status |= player->stream_settings.iec958.status[2+(n*4)] << 16;
		status |= player->stream_settings.iec958.status[3+(n*4)] << 24;
		dev_dbg(player->dev, "Channel Status Regsiter %d: %08x",
				n, status);
		set__AUD_UNIPERIF_CHANNEL_STA_REGn(player, n, status);
	}

	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0)
		set__AUD_UNIPERIF_CONFIG__CHL_STS_UPDATE(player);
	else
		set__AUD_UNIPERIF_BIT_CONTROL__CHL_STS_UPDATE(player);
}

static int uniperif_player_prepare_iec958(struct uniperif_player *player,
		struct snd_pcm_runtime *runtime)
{
	dev_dbg(player->dev, "%s(player=%p, runtime=%p)", __func__,
			player, runtime);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!runtime);

	BUG_ON(player->state != UNIPERIF_PLAYER_STATE_STOPPED);

	player->format = DEFAULT_FORMAT;
	player->oversampling = DEFAULT_OVERSAMPLING;

	if (player->conv_group) {
		player->format = snd_stm_conv_get_format(player->conv_group);

		if (UNIPERIF_PLAYER_TYPE_IS_SPDIF(player))
			BUG_ON((player->format & SND_STM_FORMAT__MASK) !=
				SND_STM_FORMAT__SPDIF);

		player->oversampling =
			snd_stm_conv_get_oversampling(player->conv_group);
		if (player->oversampling == 0)
			player->oversampling = DEFAULT_OVERSAMPLING;
	}

	dev_dbg(player->dev, "Sample frequency %d (oversampling %d)",
			runtime->rate, player->oversampling);

	BUG_ON(player->oversampling <= 0);
	BUG_ON(player->oversampling % 128);

	if (runtime->rate < MIN_IEC958_SAMPLE_RATE) {
		dev_err(player->dev, "Invalid sample rate (%d)", runtime->rate);
		return -EINVAL;
	}

	if (player->stream_settings.input_mode ==
			UNIPERIF_IEC958_INPUT_MODE_PCM) {

		dev_dbg(player->dev, "Input Mode: PCM");

		switch (runtime->format) {
		case SNDRV_PCM_FORMAT_S16_LE:
			dev_dbg(player->dev, "16-bit subframe");
			 
			set__AUD_UNIPERIF_CONFIG__MEM_FMT_16_16(player);
			 
			set__AUD_UNIPERIF_I2S_FMT__NBIT_32(player);
			 
			set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_16(player);
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			dev_dbg(player->dev, "32-bit subframe");
			 
			set__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(player);
			 
			set__AUD_UNIPERIF_I2S_FMT__NBIT_32(player);
			 
			set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_24(player);
			break;
		default:
			snd_BUG();
			return -EINVAL;
		}

		set__AUD_UNIPERIF_CONFIG__PARITY_CNTR_BY_HW(player);

		set__AUD_UNIPERIF_CONFIG__CHANNEL_STA_CNTR_BY_HW(player);

		set__AUD_UNIPERIF_CONFIG__USER_DAT_CNTR_BY_HW(player);

		set__AUD_UNIPERIF_CONFIG__VALIDITY_DAT_CNTR_BY_HW(player);

		set__AUD_UNIPERIF_CONFIG__SPDIF_SW_CTRL_DISABLE(player);

		set__AUD_UNIPERIF_CTRL__ZERO_STUFF_HW(player);

		if (player->stream_settings.encoding_mode ==
				UNIPERIF_IEC958_ENCODING_MODE_PCM) {

			dev_dbg(player->dev, "Encoding Mode: Linear PCM");

			player->stream_settings.iec958.status[4] = 0x0b;

			set__AUD_UNIPERIF_USER_VALIDITY__VALIDITY_LR(player, 0);
		} else {
			dev_dbg(player->dev, "Encoding Mode: Encoded");

			set__AUD_UNIPERIF_USER_VALIDITY__VALIDITY_LR(player, 1);
		}

		uniperif_player_set_channel_status(player, runtime);

		set__AUD_UNIPERIF_USER_VALIDITY__USER_LEFT(player, 0);
		set__AUD_UNIPERIF_USER_VALIDITY__USER_RIGHT(player, 0);
	} else {

		dev_dbg(player->dev, "Input Mode: RAW");

		set__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(player);

		set__AUD_UNIPERIF_I2S_FMT__NBIT_32(player);

		set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_32(player);

		set__AUD_UNIPERIF_CONFIG__PARITY_CNTR_BY_HW(player);

		set__AUD_UNIPERIF_CONFIG__CHANNEL_STA_CNTR_BY_SW(player);

		set__AUD_UNIPERIF_CONFIG__USER_DAT_CNTR_BY_SW(player);

		set__AUD_UNIPERIF_CONFIG__VALIDITY_DAT_CNTR_BY_SW(player);

		set__AUD_UNIPERIF_CONFIG__SPDIF_SW_CTRL_ENABLE(player);

		set__AUD_UNIPERIF_CTRL__ZERO_STUFF_HW(player);
	}

	set__AUD_UNIPERIF_CONFIG__ONE_BIT_AUD_DISABLE(player);

	set__AUD_UNIPERIF_CONFIG__REPEAT_CHL_STS_ENABLE(player);

	set__AUD_UNIPERIF_CONFIG__SUBFRAME_SEL_SUBF1_SUBF0(player);

	set__AUD_UNIPERIF_I2S_FMT__LR_POL(player, player->info->iec958_lr_pol);
	set__AUD_UNIPERIF_I2S_FMT__PADDING(player,
			player->info->iec958_i2s_mode);

	set__AUD_UNIPERIF_I2S_FMT__SCLK_EDGE_RISING(player);

	set__AUD_UNIPERIF_I2S_FMT__ALIGN_LEFT(player);

	set__AUD_UNIPERIF_I2S_FMT__ORDER_MSB(player);

	if (UNIPERIF_PLAYER_TYPE_IS_SPDIF(player))
		BUG_ON(runtime->channels != 2);

	set__AUD_UNIPERIF_I2S_FMT__NUM_CH(player, runtime->channels / 2);

	set__AUD_UNIPERIF_CTRL__ROUNDING_OFF(player);

	set__AUD_UNIPERIF_CTRL__DIVIDER(player, player->oversampling / 128);

	set__AUD_UNIPERIF_CTRL__SPDIF_LAT_OFF(player);

	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0)
		set__AUD_UNIPERIF_CTRL__SPDIF_FMT_OFF(player);
	else
		set__AUD_UNIPERIF_CTRL__SPDIF_FMT_ON(player);

	uniperif_player_set_underflow(player->substream);

	return 0;
}

static int uniperif_player_prepare(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int changed;

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!runtime);
	BUG_ON(runtime->period_size * runtime->channels >=
			MAX_SAMPLES_PER_PERIOD);

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STARTED);

	if (player->state == UNIPERIF_PLAYER_STATE_UNDERFLOW) {
		dev_err(player->dev, "Cannot prepare in underflow");
		return -EAGAIN;
	}

	changed  = (player->current_rate != runtime->rate);
	changed |= (player->current_format != runtime->format);
	changed |= (player->current_channels != runtime->channels);

	if (changed) {
		 
		if (player->state != UNIPERIF_PLAYER_STATE_STOPPED)
			uniperif_player_stop(substream);

		BUG_ON(player->state != UNIPERIF_PLAYER_STATE_STOPPED);

		player->current_rate = runtime->rate;
		player->current_format = runtime->format;
		player->current_channels = runtime->channels;

		switch (player->info->player_type) {
		case SND_STM_UNIPERIF_PLAYER_TYPE_HDMI:
			return uniperif_player_prepare_iec958(player, runtime);
		case SND_STM_UNIPERIF_PLAYER_TYPE_PCM:
			return uniperif_player_prepare_pcm(player, runtime);
		case SND_STM_UNIPERIF_PLAYER_TYPE_SPDIF:
			return uniperif_player_prepare_iec958(player, runtime);
		default:
			snd_BUG();
			return -EINVAL;
		}
	}

	return 0;
}

static int uniperif_player_start(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	unsigned long irqflags;
	int result;

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STARTED);
	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_UNDERFLOW);

	result = uniperif_player_resources_request(player);
	if (result) {
		dev_err(player->dev, "Failed to request resources");
		return result;
	}

	player->dma_descriptor = dma_audio_prep_tx_cyclic(player->dma_channel,
			substream->runtime->dma_addr, player->buffer_bytes,
			player->period_bytes);
	if (!player->dma_descriptor) {
		dev_err(player->dev, "Failed to prepare DMA descriptor");
		return -ENOMEM;
	}

	player->dma_descriptor->callback = uniperif_player_comp_cb;
	player->dma_descriptor->callback_param = player;

	if (player->state == UNIPERIF_PLAYER_STATE_PARKING) {
		 
		player->dma_cookie = dmaengine_submit(player->dma_descriptor);

		player->state = UNIPERIF_PLAYER_STATE_STARTED;
		return 0;
	}

	if (player->state == UNIPERIF_PLAYER_STATE_STANDBY) {
		 
		unsigned int ctrl = get__AUD_UNIPERIF_CTRL(player);

		ctrl &= ~(mask__AUD_UNIPERIF_CTRL__OPERATION(player) <<
			shift__AUD_UNIPERIF_CTRL__OPERATION(player));
		ctrl &= ~(mask__AUD_UNIPERIF_CTRL__EXIT_STBY_ON_EOBLOCK(player)
			<< shift__AUD_UNIPERIF_CTRL__EXIT_STBY_ON_EOBLOCK(player));

		if (UNIPERIF_PLAYER_TYPE_IS_IEC958(player))
			if (player->stream_settings.encoding_mode ==
				UNIPERIF_IEC958_ENCODING_MODE_ENCODED)
				ctrl |= 1 << shift__AUD_UNIPERIF_CTRL__EXIT_STBY_ON_EOBLOCK(player);

		ctrl |= value__AUD_UNIPERIF_CTRL__OPERATION_AUDIO_DATA(player)
			<< shift__AUD_UNIPERIF_CTRL__OPERATION(player);

		player->dma_cookie = dmaengine_submit(player->dma_descriptor);

		set__AUD_UNIPERIF_CTRL(player, ctrl);

		set__AUD_UNIPERIF_ITM_BSET__DMA_ERROR(player);
		set__AUD_UNIPERIF_ITM_BSET__FIFO_ERROR(player);

		if (player->info->underflow_enabled) {
			set__AUD_UNIPERIF_ITM_BSET__UNDERFLOW_REC_DONE(player);
			set__AUD_UNIPERIF_ITM_BSET__UNDERFLOW_REC_FAILED(player);
		}

		set__AUD_UNIPERIF_ITS_BCLR(player, get__AUD_UNIPERIF_ITS(player));

		enable_irq(player->irq);

		player->state = UNIPERIF_PLAYER_STATE_STARTED;
		return 0;
	}

	BUG_ON(player->state != UNIPERIF_PLAYER_STATE_STOPPED);

	result = snd_stm_clk_enable(player->clock);
	if (result) {
		dev_err(player->dev, "Failed to enable clock");
		return result;
	}

	result = snd_stm_clk_set_rate(player->clock,
		substream->runtime->rate * player->oversampling);
	if (result) {
		dev_err(player->dev, "Failed to set clock rate");
		snd_stm_clk_disable(player->clock);
#ifdef MY_DEF_HERE
		snd_stm_clk_unprepare(player->clock);
#endif  
		return result;
	}

	enable_irq(player->irq);

	set__AUD_UNIPERIF_ITS_BCLR(player, get__AUD_UNIPERIF_ITS(player));

	set__AUD_UNIPERIF_ITM_BSET__DMA_ERROR(player);
	set__AUD_UNIPERIF_ITM_BSET__FIFO_ERROR(player);

	if (player->info->underflow_enabled) {
		set__AUD_UNIPERIF_ITM_BSET__UNDERFLOW_REC_DONE(player);
		set__AUD_UNIPERIF_ITM_BSET__UNDERFLOW_REC_FAILED(player);
	}

	set__AUD_UNIPERIF_SOFT_RST__SOFT_RST(player);
	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0)
		while (get__AUD_UNIPERIF_SOFT_RST__SOFT_RST(player))
			udelay(5);

	player->dma_cookie = dmaengine_submit(player->dma_descriptor);

	set__AUD_UNIPERIF_SOFT_RST__SOFT_RST(player);
	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0)
		while (get__AUD_UNIPERIF_SOFT_RST__SOFT_RST(player))
			udelay(5);

	spin_lock_irqsave(&player->default_settings_lock, irqflags);

	set__AUD_UNIPERIF_CTRL__OPERATION_PCM_DATA(player);

	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0)
		if (UNIPERIF_PLAYER_TYPE_IS_IEC958(player))
			set__AUD_UNIPERIF_CTRL__SPDIF_FMT_ON(player);

	spin_unlock_irqrestore(&player->default_settings_lock, irqflags);

	player->state = UNIPERIF_PLAYER_STATE_STARTED;

	if (player->conv_group) {
		snd_stm_conv_enable(player->conv_group,
				0, substream->runtime->channels - 1);
		snd_stm_conv_unmute(player->conv_group);
	}

	return 0;
}

static int uniperif_player_stop(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STOPPED);

	if (player->conv_group) {
		snd_stm_conv_mute(player->conv_group);
		snd_stm_conv_disable(player->conv_group);
	}

	set__AUD_UNIPERIF_CTRL__OPERATION_OFF(player);

	set__AUD_UNIPERIF_SOFT_RST__SOFT_RST(player);
	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0)
		while (get__AUD_UNIPERIF_SOFT_RST__SOFT_RST(player))
			udelay(5);

	set__AUD_UNIPERIF_ITM_BCLR(player, get__AUD_UNIPERIF_ITM(player));
	disable_irq_nosync(player->irq);

	snd_stm_clk_disable(player->clock);

	if (player->dma_channel) {
		 
		dmaengine_terminate_all(player->dma_channel);
	}

	player->current_rate = 0;
	player->current_format = 0;
	player->current_channels = 0;

	player->state = UNIPERIF_PLAYER_STATE_STOPPED;
	return 0;
}

static int uniperif_player_standby(struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	int result;

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STOPPED);
	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_PARKING);
	BUG_ON(player->state == UNIPERIF_PLAYER_STATE_STANDBY);

	if (player->state == UNIPERIF_PLAYER_STATE_STARTED) {
		 
		if (player->info->parking_enabled == 1) {
			dev_dbg(player->dev, "Entering parking mode");

			result = dma_audio_parking_config(player->dma_channel,
					&player->dma_park_config);
			if (result) {
				dev_err(player->dev, "Failed to conf parking");
				goto normal_stop;
			}

			result = dma_audio_parking_enable(player->dma_channel);
			if (result) {
				dev_err(player->dev, "Failed to start parking");
				goto normal_stop;
			}

			player->state = UNIPERIF_PLAYER_STATE_PARKING;
			return 0;
		}

		if (player->info->standby_enabled == 1) {
			dev_dbg(player->dev, "Entering standby mode");

			set__AUD_UNIPERIF_ITM_BCLR(player,
						get__AUD_UNIPERIF_ITM(player));
			disable_irq_nosync(player->irq);

			set__AUD_UNIPERIF_CTRL__OPERATION_STANDBY(player);

			dmaengine_terminate_all(player->dma_channel);

			player->state = UNIPERIF_PLAYER_STATE_STANDBY;
			return 0;
		}
	}

normal_stop:
	 
	return uniperif_player_stop(substream);
}

static int uniperif_player_trigger(struct snd_pcm_substream *substream,
		int command)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);

	dev_dbg(player->dev, "%s(substream=%p)", __func__, substream);

	switch (command) {
	case SNDRV_PCM_TRIGGER_START:
		return uniperif_player_start(substream);
	case SNDRV_PCM_TRIGGER_RESUME:
		return uniperif_player_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
		return uniperif_player_standby(substream);
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return uniperif_player_stop(substream);
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		return uniperif_player_standby(substream);
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		return uniperif_player_start(substream);
	default:
		return -EINVAL;
	}
}

static snd_pcm_uframes_t uniperif_player_pointer(
		struct snd_pcm_substream *substream)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int residue, hwptr = 0;

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(!runtime);

	if (player->state != UNIPERIF_PLAYER_STATE_PARKING) {
		struct dma_tx_state state;
		enum dma_status status;

		status = player->dma_channel->device->device_tx_status(
				player->dma_channel,
				player->dma_cookie, &state);

		residue = state.residue;
		hwptr = (runtime->dma_bytes - residue) % runtime->dma_bytes;

#ifdef MY_DEF_HERE
#else  
		 
		hwptr /= player->period_bytes;
		hwptr *= player->period_bytes;
#endif  
	}

	return bytes_to_frames(runtime, hwptr);
}

static int uniperif_player_copy(struct snd_pcm_substream *substream,
		int channel, snd_pcm_uframes_t pos,
		void __user *src, snd_pcm_uframes_t count)
{
	struct uniperif_player *player = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	void *dst;
	int result;
	int i;

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));
	BUG_ON(channel != -1);
	BUG_ON(!runtime);

	dst = runtime->dma_area + frames_to_bytes(runtime, pos);

	result = copy_from_user(dst, src, frames_to_bytes(runtime, count));
	BUG_ON(result);

	if (runtime->format == SNDRV_PCM_FORMAT_S16_LE) {
		 
		if (player->info->s16_swap_lr) {
			short *dst16 = dst;
			short tmp;

			for (i = 0; i < count; ++i) {
				tmp = dst16[2 * i];
				dst16[2 * i] = dst16[2 * i + 1];
				dst16[2 * i + 1] = tmp;
			}
		}
	}

	return 0;
}

static struct snd_pcm_ops uniperif_player_pcm_ops = {
	.open =      uniperif_player_open,
	.close =     uniperif_player_close,
	.mmap =      snd_stm_buffer_mmap,
	.ioctl =     snd_pcm_lib_ioctl,
	.hw_params = uniperif_player_hw_params,
	.hw_free =   uniperif_player_hw_free,
	.prepare =   uniperif_player_prepare,
	.trigger =   uniperif_player_trigger,
	.pointer =   uniperif_player_pointer,
	.copy =      uniperif_player_copy,
};

static int uniperif_player_parking_ctl_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int uniperif_player_parking_ctl_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	ucontrol->value.integer.value[0] = player->info->parking_enabled;

	return 0;
}

static int uniperif_player_parking_ctl_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	player->info->parking_enabled = ucontrol->value.integer.value[0];

	if (player->state == UNIPERIF_PLAYER_STATE_PARKING)
		if (!player->info->parking_enabled)
			uniperif_player_stop(player->substream);

	return 0;
}

static struct snd_kcontrol_new uniperif_player_parking_ctls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = "Parking Control",
		.info = uniperif_player_parking_ctl_info,
		.get = uniperif_player_parking_ctl_get,
		.put = uniperif_player_parking_ctl_put,
	},
};

static int uniperif_player_standby_ctl_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int uniperif_player_standby_ctl_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	ucontrol->value.integer.value[0] = player->info->standby_enabled;

	return 0;
}

static int uniperif_player_standby_ctl_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	if (player->info->standby_enabled != ucontrol->value.integer.value[0]) {
		 
		player->info->standby_enabled =
			ucontrol->value.integer.value[0];

		if (!player->info->standby_enabled)
			if (player->state == UNIPERIF_PLAYER_STATE_STANDBY)
				uniperif_player_stop(player->substream);

		return 1;
	}

	return 0;
}

static struct snd_kcontrol_new uniperif_player_standby_ctls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = "Standby Control",
		.info = uniperif_player_standby_ctl_info,
		.get = uniperif_player_standby_ctl_get,
		.put = uniperif_player_standby_ctl_put,
	},
};

static int uniperif_player_underflow_ctl_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int uniperif_player_underflow_ctl_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	ucontrol->value.integer.value[0] = player->info->underflow_enabled;

	return 0;
}

static int uniperif_player_underflow_ctl_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);
	int old_underflow_enabled;

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	old_underflow_enabled = player->info->underflow_enabled;

	player->info->underflow_enabled = ucontrol->value.integer.value[0];

	if (old_underflow_enabled != player->info->underflow_enabled) {
		 
		if (player->substream)
			uniperif_player_set_underflow(player->substream);

		return 1;
	}

	return 0;
}

static int uniperif_player_underflow_us_ctl_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 10000;

	return 0;
}

static int uniperif_player_underflow_us_ctl_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	ucontrol->value.integer.value[0] = player->underflow_us;

	return 0;
}

static int uniperif_player_underflow_us_ctl_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	BUG_ON(ucontrol->value.integer.value[0] < 0);
	BUG_ON(ucontrol->value.integer.value[0] > 10000);

	if (ucontrol->value.integer.value[0] != player->underflow_us) {
		 
		player->underflow_us = ucontrol->value.integer.value[0];

		if (player->substream)
			uniperif_player_set_underflow(player->substream);

		return 1;
	}

	return 0;
}

static struct snd_kcontrol_new uniperif_player_underflow_ctls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = "Underflow Recovery Control",
		.info = uniperif_player_underflow_ctl_info,
		.get = uniperif_player_underflow_ctl_get,
		.put = uniperif_player_underflow_ctl_put,
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = "Underflow Recovery Window Adjustment",
		.info = uniperif_player_underflow_us_ctl_info,
		.get = uniperif_player_underflow_us_ctl_get,
		.put = uniperif_player_underflow_us_ctl_put,
	},
};

static int uniperif_player_ctl_iec958_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	spin_lock(&player->default_settings_lock);
	ucontrol->value.iec958 = player->stream_settings.iec958;
	spin_unlock(&player->default_settings_lock);

	return 0;
}

static int uniperif_player_ctl_iec958_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);
	int changed = 0;

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	spin_lock(&player->default_settings_lock);

	if (snd_stm_iec958_cmp(&player->default_settings.iec958,
				&ucontrol->value.iec958) != 0) {
		player->default_settings.iec958 = ucontrol->value.iec958;
		changed = 1;
	}

	if (snd_stm_iec958_cmp(&player->stream_settings.iec958,
				&ucontrol->value.iec958) != 0) {
		player->stream_settings.iec958 = ucontrol->value.iec958;
		changed = 1;

		if (player->stream_settings.iec958.status[4] == 0)
			player->stream_settings.iec958.status[4] = 0x0b;
	}

	if (changed)
		uniperif_player_set_channel_status(player, NULL);

	spin_unlock(&player->default_settings_lock);

	return changed;
}

static int uniperif_player_ctl_raw_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	spin_lock(&player->default_settings_lock);
	ucontrol->value.integer.value[0] =
			(player->default_settings.input_mode ==
			UNIPERIF_IEC958_INPUT_MODE_RAW);
	spin_unlock(&player->default_settings_lock);

	return 0;
}

static int uniperif_player_ctl_raw_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	struct snd_pcm_hardware hardware;
	enum uniperif_iec958_input_mode input_mode;

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	if (ucontrol->value.integer.value[0]) {
		hardware = uniperif_player_raw_hw;
		input_mode = UNIPERIF_IEC958_INPUT_MODE_RAW;
	} else {
		hardware = uniperif_player_pcm_hw;
		input_mode = UNIPERIF_IEC958_INPUT_MODE_PCM;
	}

	spin_lock(&player->default_settings_lock);
	changed = (input_mode != player->default_settings.input_mode);
	player->hardware = hardware;
	player->default_settings.input_mode = input_mode;
	spin_unlock(&player->default_settings_lock);

	return changed;
}

static int uniperif_player_ctl_encoded_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	spin_lock(&player->default_settings_lock);
	ucontrol->value.integer.value[0] =
			(player->default_settings.encoding_mode ==
			UNIPERIF_IEC958_ENCODING_MODE_ENCODED);
	spin_unlock(&player->default_settings_lock);

	return 0;
}

static int uniperif_player_ctl_encoded_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct uniperif_player *player = snd_kcontrol_chip(kcontrol);
	int changed = 0;
	enum uniperif_iec958_encoding_mode encoding_mode;

	dev_dbg(player->dev, "%s(kcontrol=%p, ucontrol=%p)", __func__,
			kcontrol, ucontrol);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	if (ucontrol->value.integer.value[0])
		encoding_mode = UNIPERIF_IEC958_ENCODING_MODE_ENCODED;
	else
		encoding_mode = UNIPERIF_IEC958_ENCODING_MODE_PCM;

	spin_lock(&player->default_settings_lock);

	if (encoding_mode != player->default_settings.encoding_mode) {
		player->default_settings.encoding_mode = encoding_mode;
		changed = 1;
	}

	if (encoding_mode != player->stream_settings.encoding_mode) {
		player->stream_settings.encoding_mode = encoding_mode;
		changed = 1;
	}

	if (changed) {
		set__AUD_UNIPERIF_USER_VALIDITY__VALIDITY_LR(player,
				ucontrol->value.integer.value[0]);
	}

	spin_unlock(&player->default_settings_lock);

	return changed;
}

static struct snd_kcontrol_new uniperif_player_iec958_ctls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = SNDRV_CTL_NAME_IEC958("", PLAYBACK, DEFAULT),
		.info = snd_stm_ctl_iec958_info,
		.get = uniperif_player_ctl_iec958_get,
		.put = uniperif_player_ctl_iec958_put,
	}, {
		.access = SNDRV_CTL_ELEM_ACCESS_READ,
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name =	SNDRV_CTL_NAME_IEC958("", PLAYBACK, CON_MASK),
		.info =	snd_stm_ctl_iec958_info,
		.get = snd_stm_ctl_iec958_mask_get_con,
	}, {
		.access = SNDRV_CTL_ELEM_ACCESS_READ,
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name =	SNDRV_CTL_NAME_IEC958("", PLAYBACK, PRO_MASK),
		.info =	snd_stm_ctl_iec958_info,
		.get = snd_stm_ctl_iec958_mask_get_pro,
	}, {
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = SNDRV_CTL_NAME_IEC958("Raw Data ", PLAYBACK, DEFAULT),
		.info = snd_stm_ctl_boolean_info,
		.get = uniperif_player_ctl_raw_get,
		.put = uniperif_player_ctl_raw_put,
	}, {
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = SNDRV_CTL_NAME_IEC958("Encoded Data ",
				PLAYBACK, DEFAULT),
		.info = snd_stm_ctl_boolean_info,
		.get = uniperif_player_ctl_encoded_get,
		.put = uniperif_player_ctl_encoded_put,
	}
};

#define DUMP_REGISTER(r) \
		snd_iprintf(buffer, "AUD_UNIPERIF_%-23s (offset 0x%04x) = " \
				"0x%08x\n", __stringify(r), \
				offset__AUD_UNIPERIF_##r(player), \
				get__AUD_UNIPERIF_##r(player))

static void uniperif_player_dump_registers(struct snd_info_entry *entry,
		struct snd_info_buffer *buffer)
{
	struct uniperif_player *player = entry->private_data;
	char *state[] = {
		"STOPPED",
		"STARTED",
		"PARKING",
		"STANDBY",
		"UNDERFLOW",
	};

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	snd_iprintf(buffer, "--- %s (0x%p) %s ---\n", dev_name(player->dev),
		    player->base, state[player->state]);

	DUMP_REGISTER(SOFT_RST);
	   
	DUMP_REGISTER(STA);
	DUMP_REGISTER(ITS);
	    
	DUMP_REGISTER(ITM);
	    
	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0) {
		DUMP_REGISTER(SPDIF_PA_PB);
		DUMP_REGISTER(SPDIF_PC_PD);
		DUMP_REGISTER(SPDIF_PAUSE_LAT);
		DUMP_REGISTER(SPDIF_FRAMELEN_BURST);
	}

	DUMP_REGISTER(CONFIG);
	DUMP_REGISTER(CTRL);
	DUMP_REGISTER(I2S_FMT);
	DUMP_REGISTER(STATUS_1);
	    
	DUMP_REGISTER(CHANNEL_STA_REG0);
	DUMP_REGISTER(CHANNEL_STA_REG1);
	DUMP_REGISTER(CHANNEL_STA_REG2);
	DUMP_REGISTER(CHANNEL_STA_REG3);
	DUMP_REGISTER(CHANNEL_STA_REG4);
	DUMP_REGISTER(CHANNEL_STA_REG5);
	DUMP_REGISTER(USER_VALIDITY);
	DUMP_REGISTER(DFV0);
	DUMP_REGISTER(CONTROLABILITY);
	DUMP_REGISTER(CRC_CTRL);
	DUMP_REGISTER(CRC_WINDOW);
	DUMP_REGISTER(CRC_VALUE_IN);
	DUMP_REGISTER(CRC_VALUE_OUT);

	snd_iprintf(buffer, "\n");
}

static int uniperif_player_add_ctls(struct snd_device *snd_device,
		struct snd_kcontrol_new *ctls, int num_ctls)
{
	struct uniperif_player *player = snd_device->device_data;
	struct snd_kcontrol *new_ctl;
	int result;
	int i;

	dev_dbg(player->dev, "%s(snd_device=%p, ctls=%p, num_ctls=%d)",
			__func__, snd_device, ctls, num_ctls);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	for (i = 0; i < num_ctls; ++i) {
		 
		ctls[i].device = player->info->card_device;

		new_ctl = snd_ctl_new1(&ctls[i], player);
		if (!new_ctl) {
			dev_err(player->dev, "Failed to create control");
			return -ENOMEM;
		}

		result = snd_ctl_add(snd_device->card, new_ctl);
		if (result < 0) {
			dev_err(player->dev, "Failed to add control");
			return result;
		}

		ctls[i].index++;
	}

	return 0;
}

static int uniperif_player_register(struct snd_device *snd_device)
{
	struct uniperif_player *player = snd_device->device_data;
	int result;

	dev_dbg(player->dev, "%s(snd_device=%p)", __func__, snd_device);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	set__AUD_UNIPERIF_CONFIG__BACK_STALL_REQ_DISABLE(player);
	set__AUD_UNIPERIF_CTRL__ROUNDING_OFF(player);
	set__AUD_UNIPERIF_CTRL__SPDIF_LAT_OFF(player);
	set__AUD_UNIPERIF_CONFIG__IDLE_MOD_DISABLE(player);

	player->clock = snd_stm_clk_get(player->dev, player->clk_name,
			snd_device->card, player->info->card_device);
	if (!player->clock || IS_ERR(player->clock)) {
		dev_err(player->dev, "Failed to get clock");
		return -EINVAL;
	}

	result = snd_stm_info_register(&player->proc_entry,
			dev_name(player->dev),
			uniperif_player_dump_registers, player);
	if (result < 0) {
		dev_err(player->dev, "Failed to register with procfs");
		return result;
	}

	dev_dbg(player->dev, "Adding ALSA controls");

	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0) {
		 
		result = uniperif_player_add_ctls(snd_device,
			uniperif_player_parking_ctls,
			ARRAY_SIZE(uniperif_player_parking_ctls));
		if (result) {
			dev_err(player->dev, "Failed to add parking ctls");
			return result;
		}
	}

	if (player->ver == SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0) {
		 
		result = uniperif_player_add_ctls(snd_device,
			uniperif_player_standby_ctls,
			ARRAY_SIZE(uniperif_player_standby_ctls));
		if (result) {
			dev_err(player->dev, "Failed to add standby ctls");
			return result;
		}
	}

	if (player->ver == SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0) {
		 
		result = uniperif_player_add_ctls(snd_device,
			uniperif_player_underflow_ctls,
			ARRAY_SIZE(uniperif_player_underflow_ctls));
		if (result) {
			dev_err(player->dev, "Failed to add underflow ctls");
			return result;
		}
	}

	if (UNIPERIF_PLAYER_TYPE_IS_IEC958(player)) {
		 
		result = uniperif_player_add_ctls(snd_device,
			uniperif_player_iec958_ctls,
			ARRAY_SIZE(uniperif_player_iec958_ctls));
		if (result) {
			dev_err(player->dev, "Failed to add iec958 ctls");
			return result;
		}
	}

	return result;
}

static int uniperif_player_disconnect(struct snd_device *snd_device)
{
	struct uniperif_player *player = snd_device->device_data;

	dev_dbg(player->dev, "%s(snd_device=%p)", __func__, snd_device);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	if (player->state != UNIPERIF_PLAYER_STATE_STOPPED) {
		 
		uniperif_player_stop(player->substream);

		if (player->dma_channel) {
			 
			dma_release_channel(player->dma_channel);
			player->dma_channel = NULL;
		}

		if (player->conv_group) {
			snd_stm_conv_release_group(player->conv_group);
			player->conv_group = NULL;
		}

		player->substream = NULL;
	}

	snd_stm_clk_put(player->clock);

	snd_stm_info_unregister(player->proc_entry);

	return 0;
}

static struct snd_device_ops uniperif_player_snd_device_ops = {
	.dev_register	= uniperif_player_register,
	.dev_disconnect	= uniperif_player_disconnect,
};

static int uniperif_player_parse_dt(struct platform_device *pdev,
		struct uniperif_player *player)
{
	struct snd_stm_uniperif_player_info *info;
	struct device_node *pnode;
	const char *mode;
	int val;

	BUG_ON(!pdev);
	BUG_ON(!player);

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	BUG_ON(!info);

	pnode = pdev->dev.of_node;

	of_property_read_u32(pnode, "version", &info->ver);

	val = get_property_hdl(&pdev->dev, pnode, "dmas", 0);
	if (!val) {
		dev_err(&pdev->dev, "unable to find DT dma node");
		return -EINVAL;
	}
	info->dma_np = of_find_node_by_phandle(val);

	of_property_read_string(pnode, "dma-names", &info->fdma_name);
	of_property_read_u32(pnode, "fdma-initiator", &info->fdma_initiator);
	of_property_read_u32(pnode, "fdma-direct-conn",
			&info->fdma_direct_conn);
	of_property_read_u32(pnode, "fdma-request-line",
			&info->fdma_request_line);

	of_property_read_string(pnode, "description", &info->name);
	of_property_read_u32(pnode, "card-device", &info->card_device);

	of_property_read_string(pnode, "mode", &mode);

	if (strcasecmp(mode, "hdmi") == 0)
		info->player_type = SND_STM_UNIPERIF_PLAYER_TYPE_HDMI;
	else if (strcasecmp(mode, "pcm") == 0)
		info->player_type = SND_STM_UNIPERIF_PLAYER_TYPE_PCM;
	else if (strcasecmp(mode, "spdif") == 0)
		info->player_type = SND_STM_UNIPERIF_PLAYER_TYPE_SPDIF;
	else
		info->player_type = SND_STM_UNIPERIF_PLAYER_TYPE_NONE;

	of_property_read_u32(pnode, "iec958-lr-pol", &info->iec958_lr_pol);
	of_property_read_u32(pnode, "iec958-i2s-mode", &info->iec958_i2s_mode);

	of_property_read_u32(pnode, "channels", &info->channels);
	of_property_read_u32(pnode, "parking", &info->parking_enabled);
	of_property_read_u32(pnode, "standby", &info->standby_enabled);
	of_property_read_u32(pnode, "underflow", &info->underflow_enabled);
	of_property_read_u32(pnode, "s16-swap-lr", &info->s16_swap_lr);

	player->info = info;

	of_property_read_string(pnode, "clock-names", &player->clk_name);

	if (of_property_read_u32(pnode, "auto-suspend-delay",
				 &info->suspend_delay))
		info->suspend_delay = UNIPERIF_PLAYER_NO_SUSPEND;

	return 0;
}

static int uniperif_player_probe(struct platform_device *pdev)
{
	int result = 0;
	struct uniperif_player *player;
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);
	static unsigned int channels_2_10[] = { 2, 4, 6, 8, 10 };
	static unsigned char *strings_2_10[] = {
			"2", "2/4", "2/4/6", "2/4/6/8", "2/4/6/8/10"};

	dev_dbg(&pdev->dev, "%s(pdev=%p)", __func__, pdev);

	BUG_ON(!card);

	player = devm_kzalloc(&pdev->dev, sizeof(*player), GFP_KERNEL);
	if (!player) {
		dev_err(&pdev->dev, "Failed to allocate device structure");
		return -ENOMEM;
	}
	snd_stm_magic_set(player);
	player->dev = &pdev->dev;
	player->state = UNIPERIF_PLAYER_STATE_STOPPED;
	player->underflow_us = UNIPERIF_PLAYER_UNDERFLOW_US;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "device not in dt");
		return -EINVAL;
	}

	if (uniperif_player_parse_dt(pdev, player)) {
		dev_err(&pdev->dev, "invalid dt");
		return -EINVAL;
	}

	if (!player->clk_name) {
		dev_err(&pdev->dev, "Failed to get clock name (not specify in DT)");
		return -EINVAL;
	}

	BUG_ON(!player->info);
	BUG_ON(player->info->ver == SND_STM_UNIPERIF_VERSION_UNKOWN);
	player->ver = player->info->ver;

#ifdef MY_DEF_HERE
	spin_lock_init(&player->lock);
#endif  
	spin_lock_init(&player->default_settings_lock);

	dev_notice(&pdev->dev, "'%s'", player->info->name);

	switch (player->info->player_type) {
	case SND_STM_UNIPERIF_PLAYER_TYPE_HDMI:
		player->hardware = uniperif_player_pcm_hw;
		player->stream_settings.input_mode =
				UNIPERIF_IEC958_INPUT_MODE_PCM;
		break;
	case SND_STM_UNIPERIF_PLAYER_TYPE_PCM:
		player->hardware = uniperif_player_pcm_hw;
		break;
	case SND_STM_UNIPERIF_PLAYER_TYPE_SPDIF:
		 
		player->hardware = uniperif_player_pcm_hw;
		player->stream_settings.input_mode =
				UNIPERIF_IEC958_INPUT_MODE_PCM;
		break;
	default:
		dev_err(&pdev->dev, "Invalid player type (%d)",
				player->info->player_type);
		return -EINVAL;
	}

	if (player->ver < SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0) {
		dev_notice(&pdev->dev, "Parking mode:  Supported");

		if (player->info->standby_enabled) {
			dev_warn(&pdev->dev, "Standby unsupported on old ip");
			player->info->standby_enabled = 0;
		}

		if (player->info->underflow_enabled) {
			dev_warn(&pdev->dev, "Underflow unsupported on old ip");
			player->info->underflow_enabled = 0;
		}
	}

	if (player->ver > SND_STM_UNIPERIF_VERSION_UNI_PLR_TOP_1_0) {
		dev_notice(&pdev->dev, "Standby mode:  Supported");
		dev_notice(&pdev->dev, "Underflow:     Supported");

		if (player->info->parking_enabled) {
			dev_warn(&pdev->dev, "Parking unsupported on new ip");
			player->info->parking_enabled = 0;
		}
	}

	player->default_settings.iec958.status[0] = 0x00;
	 
	player->default_settings.iec958.status[1] = 0x04;
	 
	player->default_settings.iec958.status[2] = 0x00;
	 
	player->default_settings.iec958.status[3] = 0x01;
	 
	player->default_settings.iec958.status[4] = 0x01;

	result = snd_stm_memory_request(pdev, &player->mem_region,
			&player->base);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed memory request");
		return result;
	}
	player->fifo_phys_address = player->mem_region->start +
		offset__AUD_UNIPERIF_FIFO_DATA(player);

	result = snd_stm_irq_request(pdev, &player->irq,
			uniperif_player_irq_handler, player);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed IRQ request");
		return result;
	}

	BUG_ON(player->info->channels % 2);
	BUG_ON(player->info->channels < 2);
	BUG_ON(player->info->channels > 10);

	player->channels_constraint.list = channels_2_10;
	player->channels_constraint.count = player->info->channels / 2;
	player->channels_constraint.mask = 0;

	dev_notice(player->dev, "%s-channel PCM",
			strings_2_10[player->channels_constraint.count-1]);

	result = snd_device_new(card, SNDRV_DEV_LOWLEVEL, player,
			&uniperif_player_snd_device_ops);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed to create ALSA sound device");
		goto error_device;
	}

	result = snd_pcm_new(card, NULL, player->info->card_device, 1, 0,
			&player->pcm);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed to create ALSA PCM instance");
		goto error_pcm;
	}
	player->pcm->private_data = player;
	strcpy(player->pcm->name, player->info->name);

	snd_pcm_set_ops(player->pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&uniperif_player_pcm_ops);

	player->buffer = snd_stm_buffer_create(player->pcm, player->dev,
			player->hardware.buffer_bytes_max);
	if (!player->buffer) {
		dev_err(&pdev->dev, "Failed to create buffer");
		result = -ENOMEM;
		goto error_buffer_init;
	}

	player->conv_source = snd_stm_conv_register_source(
			&platform_bus_type, pdev->dev.of_node,
			player->info->channels,
			card, player->info->card_device);
	if (!player->conv_source) {
		dev_err(&pdev->dev, "Failed to register converter source");
		result = -ENOMEM;
		goto error_conv_register_source;
	}

	platform_set_drvdata(pdev, player);

	if (player->info->suspend_delay != UNIPERIF_PLAYER_NO_SUSPEND) {
		if (player->info->suspend_delay < MIN_AUTOSUPEND_DELAY_MS) {
			player->info->suspend_delay = MIN_AUTOSUPEND_DELAY_MS;
			dev_info(&pdev->dev,
				 "auto suspend delay set to %u ms",
				 player->info->suspend_delay);
		}

		pm_runtime_set_autosuspend_delay(&pdev->dev,
						 player->info->suspend_delay);
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
	}

	return 0;

error_conv_register_source:
error_buffer_init:
	 
error_pcm:
	snd_device_free(card, player);
error_device:
	return result;
}

static int uniperif_player_remove(struct platform_device *pdev)
{
	struct uniperif_player *player = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s(pdev=%p)", __func__, pdev);

	BUG_ON(!player);
	BUG_ON(!snd_stm_magic_valid(player));

	pm_runtime_disable(&pdev->dev);

	snd_stm_conv_unregister_source(player->conv_source);

	snd_stm_magic_clear(player);

	return 0;
}

#ifdef CONFIG_PM
#ifdef MY_DEF_HERE
static int uniperif_player_runtime_suspend(struct device *dev)
#else  
static int uniperif_player_suspend(struct device *dev)
#endif  
{
	struct uniperif_player *player = dev_get_drvdata(dev);
#ifdef MY_DEF_HERE
#else  
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);
#endif  

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	if (pm_runtime_suspended(dev))
		return 0;

	if (player->dma_channel &&
	    dma_audio_is_parking_active(player->dma_channel))
		uniperif_player_stop(player->substream);

	if (player->state == UNIPERIF_PLAYER_STATE_PARKING)
		uniperif_player_stop(player->substream);

	if (player->state == UNIPERIF_PLAYER_STATE_STANDBY)
		uniperif_player_stop(player->substream);

	if (get__AUD_UNIPERIF_CTRL__OPERATION(player)) {
		dev_err(player->dev, "Cannot suspend as running");
		return -EBUSY;
	}

	if (player->state == UNIPERIF_PLAYER_STATE_STOPPED) {
		if (player->dma_channel) {
			 
			dma_release_channel(player->dma_channel);
			player->dma_channel = NULL;
		}

		if (player->conv_group) {
			 
			snd_stm_conv_release_group(player->conv_group);
			player->conv_group = NULL;
		}
	}

#ifdef MY_DEF_HERE
#else  
	 
	snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);
	snd_pcm_suspend_all(player->pcm);
#endif  

	pinctrl_pm_select_sleep_state(dev);

	return 0;
}

#ifdef MY_DEF_HERE
static int uniperif_player_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	return pinctrl_pm_select_default_state(dev);
}

#ifdef CONFIG_PM_SLEEP
static int uniperif_player_resume(struct device *dev)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	snd_power_change_state(card, SNDRV_CTL_POWER_D0);

	return uniperif_player_runtime_resume(dev);
}

static int uniperif_player_suspend(struct device *dev)
{
	int result;
	struct uniperif_player *player = dev_get_drvdata(dev);
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	result = uniperif_player_runtime_suspend(dev);

	snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);
	snd_pcm_suspend_all(player->pcm);

	return result;
}
#endif
#else  
static int uniperif_player_resume(struct device *dev)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	snd_power_change_state(card, SNDRV_CTL_POWER_D0);

	pinctrl_pm_select_default_state(dev);

	return 0;
}

static int uniperif_player_runtime_suspend(struct device *dev)
{
	int result;
	struct uniperif_player *player = dev_get_drvdata(dev);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	snd_pcm_stream_lock(player->substream);
	result = uniperif_player_suspend(dev);
	snd_pcm_stream_unlock(player->substream);

	return result;
}
#endif  

const struct dev_pm_ops uniperif_player_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(uniperif_player_suspend,
				uniperif_player_resume)

	SET_RUNTIME_PM_OPS(uniperif_player_runtime_suspend,
#ifdef MY_DEF_HERE
			   uniperif_player_runtime_resume, NULL)
#else  
			   uniperif_player_resume, NULL)
#endif  
};

#define UNIPERIF_PLAYER_PM_OPS	(&uniperif_player_pm_ops)
#else
#define UNIPERIF_PLAYER_PM_OPS	NULL
#endif

static struct of_device_id uniperif_player_match[] = {
	{
		.compatible = "st,snd_uni_player",
	},
	{},
};

MODULE_DEVICE_TABLE(of, uniperif_player_match);

static struct platform_driver uniperif_player_platform_driver = {
	.driver.name	= "snd_uni_player",
	.driver.of_match_table = uniperif_player_match,
	.driver.pm	= UNIPERIF_PLAYER_PM_OPS,
	.probe		= uniperif_player_probe,
	.remove		= uniperif_player_remove,
};

module_platform_driver(uniperif_player_platform_driver);

MODULE_AUTHOR("John Boddie <john.boddie@st.com>");
MODULE_DESCRIPTION("STMicroelectronics uniperipheral player driver");
MODULE_LICENSE("GPL");
