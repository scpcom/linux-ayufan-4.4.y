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

#define MAX_SAMPLES_PER_PERIOD ((1 << 20) - 1)

struct snd_stm_pcm_reader_info {
	const char *name;
	int ver;
	int card_device;
	int channels;
	const char *fdma_name;
	struct device_node *dma_np;
	unsigned int fdma_initiator;
	unsigned int fdma_direct_conn;
	unsigned int fdma_request_line;
};

struct uniperif_reader {
	 
	struct snd_stm_pcm_reader_info *info;
	struct device *dev;
	struct snd_pcm *pcm;
	int ver;  

	struct resource *mem_region;
	void *base;
	unsigned long fifo_phys_address;
	unsigned int irq;

	struct snd_pcm_hw_constraint_list channels_constraint;
	struct snd_stm_conv_source *conv_source;

	struct snd_stm_conv_group *conv_group;
	struct snd_stm_buffer *buffer;
	struct snd_info_entry *proc_entry;
	struct snd_pcm_substream *substream;

	int dma_max_transfer_size;
	struct st_dma_paced_config dma_config;
	struct dma_chan *dma_channel;
	struct dma_async_tx_descriptor *dma_descriptor;
	dma_cookie_t dma_cookie;

	int buffer_bytes;
	int period_bytes;

	snd_stm_magic_field;
};

static struct snd_pcm_hardware uniperif_reader_hw = {
	.info		= (SNDRV_PCM_INFO_MMAP |
				SNDRV_PCM_INFO_MMAP_VALID |
				SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER),
	.formats	= (SNDRV_PCM_FMTBIT_S32_LE),

	.rates		= SNDRV_PCM_RATE_CONTINUOUS,
	.rate_min	= 8000,
	.rate_max	= 192000,

	.channels_min	= 2,
	.channels_max	= 10,

	.periods_min	= 2,
	.periods_max	= 1024,   

	.period_bytes_min = 1280,  
	.period_bytes_max = 81920,  
	.buffer_bytes_max = 81920 * 3,  
};

static irqreturn_t uniperif_reader_irq_handler(int irq, void *dev_id)
{
	irqreturn_t result = IRQ_NONE;
	struct uniperif_reader *reader = dev_id;
	unsigned int status;

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	preempt_disable();
	status = get__AUD_UNIPERIF_ITS(reader);
	set__AUD_UNIPERIF_ITS_BCLR(reader, status);
	preempt_enable();

	if (unlikely(status & mask__AUD_UNIPERIF_ITS__FIFO_ERROR(reader))) {
		dev_err(reader->dev, "FIFO error detected");

		snd_pcm_stop(reader->substream, SNDRV_PCM_STATE_XRUN);

		result = IRQ_HANDLED;
	}

	BUG_ON(result != IRQ_HANDLED);

	return result;
}

static bool uniperif_reader_dma_filter_fn(struct dma_chan *chan, void *fn_param)
{
	struct uniperif_reader *reader = fn_param;
	struct st_dma_paced_config *config = &reader->dma_config;

	BUG_ON(!reader);

	if (reader->info->dma_np)
		if (reader->info->dma_np != chan->device->dev->of_node)
			return false;

	config->type = ST_DMA_TYPE_PACED;
	config->dma_addr = reader->fifo_phys_address;
	config->dreq_config.request_line = reader->info->fdma_request_line;
	config->dreq_config.direct_conn = reader->info->fdma_direct_conn;
	config->dreq_config.initiator = reader->info->fdma_initiator;
	config->dreq_config.increment = 0;
	config->dreq_config.hold_off = 0;
	config->dreq_config.maxburst = 1;
	config->dreq_config.buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
	config->dreq_config.direction = DMA_DEV_TO_MEM;

	chan->private = config;

	dev_dbg(reader->dev, "Using FDMA '%s' channel %d",
			dev_name(chan->device->dev), chan->chan_id);
	return true;
}

static int uniperif_reader_open(struct snd_pcm_substream *substream)
{
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	dma_cap_mask_t mask;
	int result;

	dev_dbg(reader->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));
	BUG_ON(!runtime);

	snd_pcm_set_sync(substream);   

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_cap_set(DMA_CYCLIC, mask);

	reader->dma_channel = dma_request_channel(mask,
			uniperif_reader_dma_filter_fn, reader);

	if (!reader->dma_channel) {
		dev_err(reader->dev, "Failed to request DMA channel");
		return -ENODEV;
	}

	reader->conv_group = snd_stm_conv_request_group(reader->conv_source);

	dev_dbg(reader->dev, "Attached to converter '%s'",
			reader->conv_group ?
			snd_stm_conv_get_name(reader->conv_group) : "*NONE*");

	result = snd_pcm_hw_constraint_list(runtime, 0,
			SNDRV_PCM_HW_PARAM_CHANNELS,
			&reader->channels_constraint);
	if (result < 0) {
		dev_err(reader->dev, "Failed to set channel constraint");
		goto error;
	}

	result = snd_pcm_hw_constraint_integer(runtime,
			SNDRV_PCM_HW_PARAM_PERIODS);
	if (result < 0) {
		dev_err(reader->dev, "Failed to constrain buffer periods");
		goto error;
	}

#ifdef MY_DEF_HERE
#else  
	 
	result = snd_stm_pcm_hw_constraint_transfer_bytes(runtime,
			reader->dma_max_transfer_size * 4);
	if (result < 0) {
		dev_err(reader->dev, "Failed to constrain buffer bytes");
		goto error;
	}
#endif  

	runtime->hw = uniperif_reader_hw;

	reader->substream = substream;

	return 0;

error:
	 
	if (reader->dma_channel) {
		dma_release_channel(reader->dma_channel);
		reader->dma_channel = NULL;
	}

	if (reader->conv_group) {
		snd_stm_conv_release_group(reader->conv_group);
		reader->conv_group = NULL;
	}
	return result;
}

static int uniperif_reader_close(struct snd_pcm_substream *substream)
{
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);

	dev_dbg(reader->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	if (reader->dma_channel) {
		dma_release_channel(reader->dma_channel);
		reader->dma_channel = NULL;
	}

	if (reader->conv_group) {
		snd_stm_conv_release_group(reader->conv_group);
		reader->conv_group = NULL;
	}

	reader->substream = NULL;

	return 0;
}

static int uniperif_reader_hw_free(struct snd_pcm_substream *substream)
{
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;

	dev_dbg(reader->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));
	BUG_ON(!runtime);

	if (snd_stm_buffer_is_allocated(reader->buffer)) {
		 
		dmaengine_terminate_all(reader->dma_channel);

		snd_stm_buffer_free(reader->buffer);

	}

	return 0;
}

static void uniperif_reader_dma_callback(void *param)
{
	struct uniperif_reader *reader = param;

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));
	BUG_ON(!reader->substream);

	if (!get__AUD_UNIPERIF_CTRL__OPERATION(reader))
		return;

	snd_pcm_period_elapsed(reader->substream);
}

static int uniperif_reader_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params)
{
	int result;
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int buffer_bytes, period_bytes, periods, frame_bytes, transfer_bytes;
	unsigned int transfer_size;
	struct dma_slave_config slave_config;

	dev_dbg(reader->dev, "%s(substream=%p, hw_params=%p)", __func__,
		       substream, hw_params);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));
	BUG_ON(!runtime);

	if (snd_stm_buffer_is_allocated(reader->buffer))
		uniperif_reader_hw_free(substream);

	buffer_bytes = params_buffer_bytes(hw_params);
	periods = params_periods(hw_params);
	period_bytes = buffer_bytes / periods;
	BUG_ON(periods * period_bytes != buffer_bytes);

	result = snd_stm_buffer_alloc(reader->buffer, substream, buffer_bytes);
	if (result != 0) {
		dev_err(reader->dev, "Failed to allocate %d-byte buffer",
			       buffer_bytes);
		result = -ENOMEM;
		goto error_buf_alloc;
	}

	frame_bytes = snd_pcm_format_physical_width(params_format(hw_params)) *
			params_channels(hw_params) / 8;
	transfer_bytes = snd_stm_pcm_transfer_bytes(frame_bytes,
			reader->dma_max_transfer_size * 4);
	transfer_size = transfer_bytes / 4;

	dev_dbg(reader->dev, "FDMA trigger limit %d", transfer_size);

	BUG_ON(buffer_bytes % transfer_bytes != 0);
	BUG_ON(transfer_size > reader->dma_max_transfer_size);
	BUG_ON(transfer_size != 1 && transfer_size % 2 != 0);
	BUG_ON(transfer_size >
			mask__AUD_UNIPERIF_CONFIG__FDMA_TRIGGER_LIMIT(reader));

	set__AUD_UNIPERIF_CONFIG__FDMA_TRIGGER_LIMIT(reader, transfer_size);

	reader->buffer_bytes = buffer_bytes;
	reader->period_bytes = period_bytes;

	if (!reader->dma_channel) {
		 
		dma_cap_mask_t mask;
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		dma_cap_set(DMA_CYCLIC, mask);

		reader->dma_channel = dma_request_channel(mask,
			uniperif_reader_dma_filter_fn, reader);

		if (!reader->dma_channel) {
			dev_err(reader->dev, "Failed to request DMA channel");
			return -ENODEV;
		}
	}

	if (!reader->conv_group) {
		 
		reader->conv_group =
			snd_stm_conv_request_group(reader->conv_source);

		dev_dbg(reader->dev, "Attached to converter '%s'",
			reader->conv_group ?
			snd_stm_conv_get_name(reader->conv_group) : "*NONE*");
	}

	slave_config.direction = DMA_DEV_TO_MEM;
	slave_config.src_addr = reader->fifo_phys_address;
	slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	slave_config.src_maxburst = transfer_size;

	result = dmaengine_slave_config(reader->dma_channel, &slave_config);
	if (result) {
		dev_err(reader->dev, "Failed to configure DMA channel");
		goto error_dma_config;
	}

	return 0;

error_dma_config:
	snd_stm_buffer_free(reader->buffer);
error_buf_alloc:
	return result;
}

static int uniperif_reader_prepare(struct snd_pcm_substream *substream)
{
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int format = DEFAULT_FORMAT;
	unsigned int lr_pol;

	dev_dbg(reader->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));
	BUG_ON(!runtime);
	BUG_ON(runtime->period_size * runtime->channels >=
			MAX_SAMPLES_PER_PERIOD);

	if (reader->conv_group)
		format = snd_stm_conv_get_format(reader->conv_group);

	switch (format & SND_STM_FORMAT__SUBFRAME_MASK) {
	case SND_STM_FORMAT__SUBFRAME_32_BITS:
		dev_dbg(reader->dev, "32-bit subframe");
		set__AUD_UNIPERIF_I2S_FMT__NBIT_32(reader);
		set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_24(reader);
		break;
	case SND_STM_FORMAT__SUBFRAME_16_BITS:
		dev_dbg(reader->dev, "16-bit subframe");
		set__AUD_UNIPERIF_I2S_FMT__NBIT_16(reader);
		set__AUD_UNIPERIF_I2S_FMT__DATA_SIZE_16(reader);
		break;
	default:
		snd_BUG();
		return -EINVAL;
	}

	switch (format & SND_STM_FORMAT__MASK) {
	case SND_STM_FORMAT__I2S:
		dev_dbg(reader->dev, "I2S format");
		set__AUD_UNIPERIF_I2S_FMT__ALIGN_LEFT(reader);
		set__AUD_UNIPERIF_I2S_FMT__PADDING_I2S_MODE(reader);
		lr_pol = value__AUD_UNIPERIF_I2S_FMT__LR_POL_LOW(reader);
		break;
	case SND_STM_FORMAT__LEFT_JUSTIFIED:
		dev_dbg(reader->dev, "Left Justifed format");
		set__AUD_UNIPERIF_I2S_FMT__ALIGN_LEFT(reader);
		set__AUD_UNIPERIF_I2S_FMT__PADDING_SONY_MODE(reader);
		lr_pol = value__AUD_UNIPERIF_I2S_FMT__LR_POL_HIG(reader);
		break;
	case SND_STM_FORMAT__RIGHT_JUSTIFIED:
		dev_dbg(reader->dev, "Right Justifed format");
		set__AUD_UNIPERIF_I2S_FMT__ALIGN_RIGHT(reader);
		set__AUD_UNIPERIF_I2S_FMT__PADDING_SONY_MODE(reader);
		lr_pol = value__AUD_UNIPERIF_I2S_FMT__LR_POL_HIG(reader);
		break;
	default:
		snd_BUG();
		return -EINVAL;
	}

	switch (runtime->format) {
	case SNDRV_PCM_FORMAT_S32_LE:
		 
		set__AUD_UNIPERIF_CONFIG__MEM_FMT_16_0(reader);

		set__AUD_UNIPERIF_I2S_FMT__LR_POL(reader, lr_pol);
		break;

	default:
		snd_BUG();
		return -EINVAL;
	}

	set__AUD_UNIPERIF_CONFIG__MSTR_CLKEDGE_RISING(reader);
	set__AUD_UNIPERIF_CTRL__READER_OUT_SEL_IN_MEM(reader);

	set__AUD_UNIPERIF_I2S_FMT__ORDER_MSB(reader);

	set__AUD_UNIPERIF_I2S_FMT__SCLK_EDGE_RISING(reader);

	BUG_ON(runtime->channels % 2 != 0);
	BUG_ON(runtime->channels < 2);
	BUG_ON(runtime->channels > 10);

	set__AUD_UNIPERIF_I2S_FMT__NUM_CH(reader, runtime->channels / 2);

	return 0;
}

static int uniperif_reader_start(struct snd_pcm_substream *substream)
{
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);

	dev_dbg(reader->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	set__AUD_UNIPERIF_SOFT_RST__SOFT_RST(reader);
	while (get__AUD_UNIPERIF_SOFT_RST__SOFT_RST(reader))
		udelay(5);

	if (!reader->dma_channel) {
		 
		dma_cap_mask_t mask;
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		dma_cap_set(DMA_CYCLIC, mask);

		reader->dma_channel = dma_request_channel(mask,
			uniperif_reader_dma_filter_fn, reader);

		if (!reader->dma_channel) {
			dev_err(reader->dev, "Failed to request DMA channel");
			return -ENODEV;
		}
	}

	if (!reader->conv_group) {
		 
		reader->conv_group =
			snd_stm_conv_request_group(reader->conv_source);

		dev_dbg(reader->dev, "Attached to converter '%s'",
			reader->conv_group ?
			snd_stm_conv_get_name(reader->conv_group) : "*NONE*");
	}

	BUG_ON(!reader->dma_channel->device->device_prep_dma_cyclic);
	reader->dma_descriptor =
		reader->dma_channel->device->device_prep_dma_cyclic(
			reader->dma_channel, substream->runtime->dma_addr,
			reader->buffer_bytes, reader->period_bytes,
			DMA_DEV_TO_MEM, 0x0, NULL);
	if (!reader->dma_descriptor) {
		dev_err(reader->dev, "Failed to prepare DMA descriptor");
		return -ENOMEM;
	}

	reader->dma_descriptor->callback = uniperif_reader_dma_callback;
	reader->dma_descriptor->callback_param = reader;

	reader->dma_cookie = dmaengine_submit(reader->dma_descriptor);

	enable_irq(reader->irq);
	set__AUD_UNIPERIF_ITS_BCLR__FIFO_ERROR(reader);
	set__AUD_UNIPERIF_ITM_BSET__FIFO_ERROR(reader);

	set__AUD_UNIPERIF_CTRL__OPERATION_PCM_DATA(reader);

	if (reader->conv_group) {
		snd_stm_conv_enable(reader->conv_group,
				0, substream->runtime->channels - 1);
		snd_stm_conv_unmute(reader->conv_group);
	}

	return 0;
}

static int uniperif_reader_stop(struct snd_pcm_substream *substream)
{
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);

	dev_dbg(reader->dev, "%s(substream=%p)", __func__, substream);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	if (reader->conv_group) {
		snd_stm_conv_mute(reader->conv_group);
		snd_stm_conv_disable(reader->conv_group);
	}

	set__AUD_UNIPERIF_ITM_BCLR__FIFO_ERROR(reader);
	disable_irq_nosync(reader->irq);

	dmaengine_terminate_all(reader->dma_channel);

	set__AUD_UNIPERIF_CTRL__OPERATION_OFF(reader);

	return 0;
}

static int uniperif_reader_trigger(struct snd_pcm_substream *substream,
		int command)
{
	switch (command) {
	case SNDRV_PCM_TRIGGER_START:
		return uniperif_reader_start(substream);
	case SNDRV_PCM_TRIGGER_RESUME:
		return uniperif_reader_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
		return uniperif_reader_stop(substream);
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return uniperif_reader_stop(substream);
	default:
		return -EINVAL;
	}
}

static snd_pcm_uframes_t uniperif_reader_pointer(
		struct snd_pcm_substream *substream)
{
	struct uniperif_reader *reader = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int residue, hwptr;
	snd_pcm_uframes_t pointer;
	struct dma_tx_state state;
	enum dma_status status;

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));
	BUG_ON(!runtime);

	status = reader->dma_channel->device->device_tx_status(
			reader->dma_channel,
			reader->dma_cookie, &state);

	residue = state.residue;

	hwptr = (runtime->dma_bytes - residue) % runtime->dma_bytes;
	pointer = bytes_to_frames(runtime, hwptr);

	return pointer;
}

static struct snd_pcm_ops uniperif_reader_pcm_ops = {
	.open =      uniperif_reader_open,
	.close =     uniperif_reader_close,
	.mmap =      snd_stm_buffer_mmap,
	.ioctl =     snd_pcm_lib_ioctl,
	.hw_params = uniperif_reader_hw_params,
	.hw_free =   uniperif_reader_hw_free,
	.prepare =   uniperif_reader_prepare,
	.trigger =   uniperif_reader_trigger,
	.pointer =   uniperif_reader_pointer,
};

#define DUMP_REGISTER(r) \
		snd_iprintf(buffer, "AUD_UNIPERIF_%-23s (offset 0x%04x) = " \
				"0x%08x\n", __stringify(r), \
				offset__AUD_UNIPERIF_##r(reader), \
				get__AUD_UNIPERIF_##r(reader))

static void uniperif_reader_dump_registers(struct snd_info_entry *entry,
		struct snd_info_buffer *buffer)
{
	struct uniperif_reader *reader = entry->private_data;

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	snd_iprintf(buffer, "--- %s (0x%p) ---\n", dev_name(reader->dev),
			reader->base);

	DUMP_REGISTER(SOFT_RST);
	 
	DUMP_REGISTER(STA);
	DUMP_REGISTER(ITS);
	 
	DUMP_REGISTER(ITM);
	 
	DUMP_REGISTER(CONFIG);
	DUMP_REGISTER(CTRL);
	DUMP_REGISTER(I2S_FMT);
	DUMP_REGISTER(STATUS_1);
	DUMP_REGISTER(DFV0);
	DUMP_REGISTER(CONTROLABILITY);
	DUMP_REGISTER(CRC_CTRL);
	DUMP_REGISTER(CRC_WINDOW);
	DUMP_REGISTER(CRC_VALUE_IN);
	DUMP_REGISTER(CRC_VALUE_OUT);

	snd_iprintf(buffer, "\n");
}

static int uniperif_reader_register(struct snd_device *snd_device)
{
	struct uniperif_reader *reader = snd_device->device_data;

	dev_dbg(reader->dev, "%s(snd_device=%p)", __func__, snd_device);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	set__AUD_UNIPERIF_CONFIG__BACK_STALL_REQ_DISABLE(reader);
	set__AUD_UNIPERIF_CTRL__ROUNDING_OFF(reader);
	set__AUD_UNIPERIF_CTRL__SPDIF_LAT_OFF(reader);
	set__AUD_UNIPERIF_CONFIG__IDLE_MOD_DISABLE(reader);

	return snd_stm_info_register(&reader->proc_entry,
			dev_name(reader->dev),
			uniperif_reader_dump_registers, reader);
}

static int uniperif_reader_disconnect(struct snd_device *snd_device)
{
	struct uniperif_reader *reader = snd_device->device_data;

	dev_dbg(reader->dev, "%s(snd_device=%p)", __func__, snd_device);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	snd_stm_info_unregister(reader->proc_entry);

	return 0;
}

static struct snd_device_ops uniperif_reader_snd_device_ops = {
	.dev_register	= uniperif_reader_register,
	.dev_disconnect	= uniperif_reader_disconnect,
};

static int uniperif_reader_parse_dt(struct platform_device *pdev,
		struct uniperif_reader *reader)
{
	struct snd_stm_pcm_reader_info *info;
	struct device_node *pnode;
	int val;

	BUG_ON(!pdev);
	BUG_ON(!reader);

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
	of_property_read_u32(pnode, "channels", &info->channels);

	reader->info = info;

	return 0;
}

static int uniperif_reader_probe(struct platform_device *pdev)
{
	int result = 0;
	struct uniperif_reader *reader;
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);
	static unsigned int channels_2_10[] = { 2, 4, 6, 8, 10 };
	static unsigned char *strings_2_10[] = {
			"2", "2/4", "2/4/6", "2/4/6/8", "2/4/6/8/10"};

	dev_dbg(&pdev->dev, "%s(pdev=%p)", __func__, pdev);

	BUG_ON(!card);

	reader = devm_kzalloc(&pdev->dev, sizeof(*reader), GFP_KERNEL);
	if (!reader) {
		dev_err(&pdev->dev, "Failed to allocate device structure");
		return -ENOMEM;
	}
	snd_stm_magic_set(reader);
	reader->dev = &pdev->dev;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "device not in dt");
		return -EINVAL;
	}

	if (uniperif_reader_parse_dt(pdev, reader)) {
		dev_err(&pdev->dev, "invalid dt");
		return -EINVAL;
	}

	BUG_ON(!reader->info);
	BUG_ON(reader->info->ver == SND_STM_UNIPERIF_VERSION_UNKOWN);
	reader->ver = reader->info->ver;

	dev_notice(&pdev->dev, "'%s'", reader->info->name);

	result = snd_stm_memory_request(pdev, &reader->mem_region,
			&reader->base);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed memory request");
		return result;
	}
	reader->fifo_phys_address = reader->mem_region->start +
		offset__AUD_UNIPERIF_FIFO_DATA(reader);

	result = snd_stm_irq_request(pdev, &reader->irq,
			uniperif_reader_irq_handler, reader);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed IRQ request");
		return result;
	}

	reader->dma_max_transfer_size = 40;

	BUG_ON(reader->info->channels <= 0);
	BUG_ON(reader->info->channels > 10);
	BUG_ON(reader->info->channels % 2 != 0);

	reader->channels_constraint.list = channels_2_10;
	reader->channels_constraint.count = reader->info->channels / 2;
	reader->channels_constraint.mask = 0;

	dev_notice(reader->dev, "%s-channel PCM",
			strings_2_10[reader->channels_constraint.count-1]);

	result = snd_device_new(card, SNDRV_DEV_LOWLEVEL, reader,
			&uniperif_reader_snd_device_ops);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed to create ALSA sound device");
		goto error_device;
	}

	result = snd_pcm_new(card, NULL, reader->info->card_device, 0, 1,
			&reader->pcm);
	if (result < 0) {
		dev_err(&pdev->dev, "Failed to create ALSA PCM instance");
		goto error_pcm;
	}
	reader->pcm->private_data = reader;
	strcpy(reader->pcm->name, reader->info->name);

	snd_pcm_set_ops(reader->pcm, SNDRV_PCM_STREAM_CAPTURE,
			&uniperif_reader_pcm_ops);

	reader->buffer = snd_stm_buffer_create(reader->pcm, reader->dev,
			uniperif_reader_hw.buffer_bytes_max);
	if (!reader->buffer) {
		dev_err(&pdev->dev, "Failed to create buffer");
		result = -ENOMEM;
		goto error_buffer_init;
	}

	reader->conv_source = snd_stm_conv_register_source(
			&platform_bus_type, pdev->dev.of_node,
			reader->info->channels,
			card, reader->info->card_device);
	if (!reader->conv_source) {
		dev_err(&pdev->dev, "Failed to register converter source");
		result = -ENOMEM;
		goto error_conv_register_source;
	}

	platform_set_drvdata(pdev, reader);

	pm_runtime_enable(&pdev->dev);

	return 0;

error_conv_register_source:
error_buffer_init:
	 
error_pcm:
	snd_device_free(card, reader);
error_device:
	return result;
}

static int uniperif_reader_remove(struct platform_device *pdev)
{
	struct uniperif_reader *reader = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s(pdev=%p)", __func__, pdev);

	BUG_ON(!reader);
	BUG_ON(!snd_stm_magic_valid(reader));

	pm_runtime_disable(&pdev->dev);

	snd_stm_conv_unregister_source(reader->conv_source);

	snd_stm_magic_clear(reader);

	return 0;
}

#ifdef MY_DEF_HERE
#ifdef CONFIG_PM_SLEEP
static int uniperif_reader_suspend(struct device *dev)
{
	struct uniperif_reader *reader = dev_get_drvdata(dev);
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	if (get__AUD_UNIPERIF_CTRL__OPERATION(reader)) {
		dev_err(reader->dev, "Cannot suspend as running");
		return -EBUSY;
	}

	if (reader->dma_channel) {
		dma_release_channel(reader->dma_channel);
		reader->dma_channel = NULL;
	}

	if (reader->conv_group) {
		snd_stm_conv_release_group(reader->conv_group);
		reader->conv_group = NULL;
	}

	snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);
	snd_pcm_suspend_all(reader->pcm);

	pinctrl_pm_select_sleep_state(dev);

	return 0;
}

static int uniperif_reader_resume(struct device *dev)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	snd_power_change_state(card, SNDRV_CTL_POWER_D0);

	pinctrl_pm_select_default_state(dev);

	return 0;
}

SIMPLE_DEV_PM_OPS(uniperif_reader_pm_ops,
		uniperif_reader_suspend,
		uniperif_reader_resume);

#define UNIPERIF_READER_PM_OPS	(&uniperif_reader_pm_ops)
#else
#define UNIPERIF_READER_PM_OPS	NULL
#endif
#else  
#ifdef CONFIG_PM
static int uniperif_reader_suspend(struct device *dev)
{
	struct uniperif_reader *reader = dev_get_drvdata(dev);
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	if (get__AUD_UNIPERIF_CTRL__OPERATION(reader)) {
		dev_err(reader->dev, "Cannot suspend as running");
		return -EBUSY;
	}

	if (reader->dma_channel) {
		dma_release_channel(reader->dma_channel);
		reader->dma_channel = NULL;
	}

	if (reader->conv_group) {
		snd_stm_conv_release_group(reader->conv_group);
		reader->conv_group = NULL;
	}

	snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);
	snd_pcm_suspend_all(reader->pcm);

	pinctrl_pm_select_sleep_state(dev);

	return 0;
}

static int uniperif_reader_resume(struct device *dev)
{
	struct snd_card *card = snd_stm_card_get(SND_STM_CARD_TYPE_AUDIO);

	dev_dbg(dev, "%s(dev=%p)", __func__, dev);

	snd_power_change_state(card, SNDRV_CTL_POWER_D0);

	pinctrl_pm_select_default_state(dev);

	return 0;
}

SIMPLE_DEV_PM_OPS(uniperif_reader_pm_ops,
		uniperif_reader_suspend,
		uniperif_reader_resume);

#define UNIPERIF_READER_PM_OPS	(&uniperif_reader_pm_ops)
#else
#define UNIPERIF_READER_PM_OPS	NULL
#endif
#endif  

static struct of_device_id uniperif_reader_match[] = {
	{
		.compatible = "st,snd_uni_reader",
	},
	{},
};

MODULE_DEVICE_TABLE(of, uniperif_reader_match);

static struct platform_driver uniperif_reader_platform_driver = {
	.driver.name = "snd_uni_reader",
	.driver.of_match_table = uniperif_reader_match,
	.driver.pm	= UNIPERIF_READER_PM_OPS,
	.probe = uniperif_reader_probe,
	.remove = uniperif_reader_remove,
};

module_platform_driver(uniperif_reader_platform_driver);

MODULE_AUTHOR("John Boddie <john.boddie@st.com>");
MODULE_DESCRIPTION("STMicroelectronics uniperipheral reader driver");
MODULE_LICENSE("GPL");
