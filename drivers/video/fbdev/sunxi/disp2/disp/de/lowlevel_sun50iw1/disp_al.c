/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "disp_al.h"
#include "de_hal.h"

int disp_exit_al(void)
{
	return 0;
}

bool disp_al_get_direct_show_state(unsigned int disp)
{
	return 0;
}

int disp_al_hdmi_irq_disable(u32 screen_id)
{
	return 0;
}

int disp_al_hdmi_irq_enable(u32 screen_id)
{
	return 0;
}

struct disp_al_private_data {
	u32 output_type[DEVICE_NUM];/* index according to device */
	u32 output_mode[DEVICE_NUM];/* indicate mode for tv/hdmi, lcd_if for lcd */
	u32 output_cs[DEVICE_NUM];/* index according to device */
	u32 disp_device[DEVICE_NUM];/* disp_device[0]=1:indicate disp0 <->device1 */
	u32 disp_disp[DEVICE_NUM];/* disp_disp[0]=1: indicate device0 <-> disp1 */
	struct disp_rect disp_size[DEVICE_NUM];
	unsigned long long logo_addr[DEVICE_NUM][3];
	struct disp_rect logo_size[DEVICE_NUM];
	struct disp_layer_info logo_info[DEVICE_NUM];
	u32 output_fps[DEVICE_NUM];/* index according to device */
	u32 tcon_index[DEVICE_NUM];
};



static struct disp_al_private_data al_priv;

extern int bsp_disp_feat_get_num_layers(unsigned int screen_id);
extern int bsp_disp_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn);
int disp_checkout_straight(unsigned int disp, struct disp_layer_config_data *data)
{
	unsigned char i, chn, vi_chn, device_type;
	struct disp_layer_config_data *pdata;
	u32 num_layers = bsp_disp_feat_get_num_layers(disp);
	u32 num_layers_video_by_chn[4] = {0};
	u32 num_layers_video = 0;
	u32 index = 0;

	chn = de_feat_get_num_chns(disp);

	vi_chn = de_feat_get_num_vi_chns(disp);
	for (i = 0; i < vi_chn; i++) {
		num_layers_video_by_chn[i] = bsp_disp_feat_get_num_layers_by_chn(disp, i);
	}

	for (i = 0; i < vi_chn; i++) {
		num_layers_video += bsp_disp_feat_get_num_layers_by_chn(disp, i);
	}

	pdata = data;

	device_type = al_priv.output_type[al_priv.disp_device[disp]];

	/* printk("<7>%s: device type = %d\n",__func__, device_type); */

	if (device_type == DISP_OUTPUT_TYPE_TV)			/* this type for extern cvbs */{
		int j;

		for (j = 0; j < vi_chn; j++) {
			for (i = 0; i < num_layers_video_by_chn[j]; i++) {
				if (pdata->config.enable &&
					pdata->config.info.fb.format >= DISP_FORMAT_YUV444_I_AYUV){
					pdata = data + num_layers_video_by_chn[j] + index;
					index += num_layers_video_by_chn[j];
					break;
				}
				pdata++;
			}
		}

		if (index < num_layers_video) {
		       return -1;
		}
		for (i = index; i < num_layers; i++) {
		       if (pdata->config.enable) {
			       index++;
			       break;
		       }
		       pdata++;
		}
		if (index > num_layers_video) {
		       return -1;
		}
	} else
		return -1;

	return 0;
}


int disp_al_layer_apply(unsigned int disp, struct disp_layer_config_data *data, unsigned int layer_num)
{
	return de_al_lyr_apply(disp, data, layer_num, al_priv.output_type[al_priv.disp_device[disp]]);
}

int disp_al_manager_init(unsigned int disp)
{
	return de_clk_enable(DE_CLK_CORE0 + disp);
}

int disp_al_manager_exit(unsigned int disp)
{
	return de_clk_disable(DE_CLK_CORE0 + disp);
}

int disp_al_manager_apply(unsigned int disp, struct disp_manager_data *data)
{
	if (data->flag & MANAGER_ENABLE_DIRTY) {
		al_priv.disp_size[disp].width = data->config.size.width;
		al_priv.disp_size[disp].height = data->config.size.height;
		al_priv.disp_device[disp] = data->config.hwdev_index;
		al_priv.disp_disp[al_priv.disp_device[disp]] = disp;
		al_priv.output_cs[al_priv.disp_device[disp]] = data->config.cs;
	}

	if (al_priv.output_type[al_priv.disp_device[disp]] == (u32)DISP_OUTPUT_TYPE_HDMI) {
		if (data->config.cs != 0)/* YUV output */
			tcon1_hdmi_color_remap(al_priv.disp_device[disp], 1);
		else
			tcon1_hdmi_color_remap(al_priv.disp_device[disp], 0);
	}
	de_update_de_frequency(data->config.de_freq);

	return de_al_mgr_apply(disp, data);
}

int disp_al_manager_sync(unsigned int disp)
{
	return de_al_mgr_sync(disp);
}


int disp_al_manager_update_regs(unsigned int disp)
{
	return de_al_mgr_update_regs(disp);
}

int disp_al_manager_query_irq(unsigned int disp)
{
	return de_al_query_irq(disp);
}


int disp_al_manager_enable_irq(unsigned int disp)
{
	return de_al_enable_irq(disp, 1);
}

int disp_al_manager_disable_irq(unsigned int disp)
{
	return de_al_enable_irq(disp, 0);
}


int disp_al_enhance_apply(unsigned int disp, struct disp_enhance_config *config)
{
	if (config->flags & ENH_MODE_DIRTY) {
		struct disp_csc_config csc_config;

		de_dcsc_get_config(disp, &csc_config);
		csc_config.enhance_mode = (config->info.mode >> 16);
		de_dcsc_apply(disp, &csc_config);
	}

	return de_enhance_apply(disp, config);
}

#if 1


int disp_al_enhance_update_regs(unsigned int disp)
{
	return de_enhance_update_regs(disp);
}

int disp_al_enhance_sync(unsigned int disp)
{
	return de_enhance_sync(disp);
}

int disp_al_enhance_tasklet(unsigned int disp)
{
	return de_enhance_tasklet(disp);
}


int disp_al_capture_init(unsigned int disp)
{
	int ret = -1;

	ret = de_clk_enable(DE_CLK_WB);
	WB_EBIOS_DeReset(disp);
	ret = wb_input_select(disp);
	return ret;
}


int disp_al_capture_exit(unsigned int disp)
{
	WB_EBIOS_Reset(disp);
	de_clk_disable(DE_CLK_WB);
	return 0;
}

int disp_al_capture_sync(u32 disp)
{
	WB_EBIOS_Update_Regs(disp);
	WB_EBIOS_Writeback_Enable(disp, 1);
	return 0;
}

int disp_al_capture_apply(unsigned int disp, struct disp_capture_config *cfg)
{
	return WB_EBIOS_Apply(disp, cfg);
}

int disp_al_capture_get_status(unsigned int disp)
{
	return WB_EBIOS_Get_Status(disp);
}

int disp_al_smbl_apply(unsigned int disp, struct disp_smbl_info *info)
{
	return de_smbl_apply(disp, info);
}

int disp_al_smbl_update_regs(unsigned int disp)
{
	return de_smbl_update_regs(disp);
}

int disp_al_smbl_sync(unsigned int disp)
{
	return 0;
}

int disp_al_smbl_tasklet(unsigned int disp)
{
	return de_smbl_tasklet(disp);
}

int disp_al_smbl_get_status(unsigned int disp)
{
	return de_smbl_get_status(disp);
}

static struct lcd_clk_info clk_tbl[] = {
	{LCD_IF_HV,    6, 1, 1, 0},
	{LCD_IF_CPU,   12, 1, 1, 0},
	{LCD_IF_LVDS,   7, 1, 1, 0},
	{LCD_IF_DSI,    4, 1, 4, 148500000},
};
/* lcd */
/* lcd_dclk_freq * div -> lcd_clk_freq * div2 -> pll_freq */
/* lcd_dclk_freq * dsi_div -> lcd_dsi_freq */
int disp_al_lcd_get_clk_info(u32 screen_id, struct lcd_clk_info *info, struct disp_panel_para *panel)
{
	int tcon_div = 6;/* tcon inner div */
	int lcd_div = 1;/* lcd clk div */
	int dsi_div = 4;/* dsi clk div */
	int dsi_rate = 0;
	int i;
	int find = 0;

	if (panel == NULL) {
		__wrn("panel is NULL\n");
		return 0;
	}

	for (i = 0; i < sizeof(clk_tbl)/sizeof(struct lcd_clk_info); i++) {
		if (clk_tbl[i].lcd_if == panel->lcd_if) {
			tcon_div = clk_tbl[i].tcon_div;
			lcd_div = clk_tbl[i].lcd_div;
			dsi_div = clk_tbl[i].dsi_div;
			dsi_rate = clk_tbl[i].dsi_rate;
			find = 1;
			break;
		}
	}

	if (panel->lcd_if == LCD_IF_DSI) {
		u32 lane = panel->lcd_dsi_lane;
		u32 bitwidth = 0;

		switch (panel->lcd_dsi_format) {
		case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
		case LCD_DSI_FORMAT_RGB666P:
			bitwidth = 18;
			break;
		}

		dsi_div = bitwidth/lane;
	}

	if (find == 0)
		__wrn("cant find clk info for lcd_if %d\n", panel->lcd_if);

	info->tcon_div = tcon_div;
	info->lcd_div = lcd_div;
	info->dsi_div = dsi_div;
	info->dsi_rate = dsi_rate;

	return 0;
}

int disp_al_lcd_cfg(u32 screen_id, struct disp_panel_para *panel, struct panel_extend_para *extend_panel)
{
	struct lcd_clk_info info;

	al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_LCD;
	al_priv.output_mode[screen_id] = (u32)panel->lcd_if;
	al_priv.output_fps[screen_id] = panel->lcd_dclk_freq * 1000000 / panel->lcd_ht / panel->lcd_vt;
	al_priv.tcon_index[screen_id] = 0;

	de_update_device_fps(al_priv.disp_disp[screen_id], al_priv.output_fps[screen_id]);

	tcon_init(screen_id);
	disp_al_lcd_get_clk_info(screen_id, &info, panel);
	tcon0_set_dclk_div(screen_id, info.tcon_div);

	if (tcon0_cfg(screen_id, panel) != 0)
		DE_WRN("lcd cfg fail!\n");
	else
		DE_INF("lcd cfg ok!\n");

	tcon0_cfg_ext(screen_id, extend_panel);

	if (panel->lcd_if == LCD_IF_DSI)	{
#if defined(SUPPORT_DSI)
		if (dsi_cfg(screen_id, panel) != 0) {
			DE_WRN("dsi cfg fail!\n");
		}
#endif
	}

	return 0;
}

int disp_al_lcd_cfg_ext(u32 screen_id, struct panel_extend_para *extend_panel)
{
	tcon0_cfg_ext(screen_id, extend_panel);

	return 0;
}

int disp_al_lcd_enable(u32 screen_id, struct disp_panel_para *panel)
{
	tcon0_open(screen_id, panel);
	if (panel->lcd_if == LCD_IF_LVDS) {
		lvds_open(screen_id, panel);
	} else if (panel->lcd_if == LCD_IF_DSI) {
#if defined(SUPPORT_DSI)
		dsi_open(screen_id, panel);
#endif
	}

	return 0;
}

int disp_al_lcd_disable(u32 screen_id, struct disp_panel_para *panel)
{
	al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_NONE;

	if (panel->lcd_if == LCD_IF_LVDS) {
		lvds_close(screen_id);
	} else if (panel->lcd_if == LCD_IF_DSI) {
#if defined(SUPPORT_DSI)
		dsi_close(screen_id);
#endif
	}
	tcon0_close(screen_id);
	tcon_exit(screen_id);

	return 0;
}


/* query lcd irq, clear it when the irq queried exist
 */
int disp_al_lcd_query_irq(u32 screen_id, enum __lcd_irq_id_t irq_id, struct disp_panel_para *panel)
{
	int ret = 0;

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI &&
	    panel->lcd_dsi_if != LCD_DSI_IF_COMMAND_MODE) {
		enum __dsi_irq_id_t dsi_irq = (irq_id == LCD_IRQ_TCON0_VBLK)
						  ? DSI_IRQ_VIDEO_VBLK
						  : DSI_IRQ_VIDEO_LINE;

		return dsi_irq_query(screen_id, dsi_irq);
	} else
#endif
		return tcon_irq_query(screen_id, irq_id);

	return ret;
}

int disp_al_lcd_enable_irq(u32 screen_id, enum __lcd_irq_id_t irq_id, struct disp_panel_para *panel)
{
	int ret = 0;

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI) {
		enum __dsi_irq_id_t dsi_irq = (irq_id == LCD_IRQ_TCON0_VBLK)?DSI_IRQ_VIDEO_VBLK:DSI_IRQ_VIDEO_LINE;

		ret = dsi_irq_enable(screen_id, dsi_irq);
	} else
#endif
	ret = tcon_irq_enable(screen_id, irq_id);

	return ret;
}

int disp_al_lcd_disable_irq(u32 screen_id, enum __lcd_irq_id_t irq_id, struct disp_panel_para *panel)
{
	int ret = 0;

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI) {
		enum __dsi_irq_id_t dsi_irq = (irq_id == LCD_IRQ_TCON0_VBLK)?DSI_IRQ_VIDEO_VBLK:DSI_IRQ_VIDEO_LINE;

		ret = dsi_irq_disable(screen_id, dsi_irq);
	} else
#endif
	ret = tcon_irq_disable(screen_id, irq_id);

	return ret;
}

int disp_al_lcd_tri_busy(u32 screen_id, struct disp_panel_para *panel)
{
	int busy = 0;
	int ret = 0;

	busy |= tcon0_tri_busy(screen_id);
#if defined(SUPPORT_DSI)
	busy |= dsi_inst_busy(screen_id);
#endif
	ret = (busy == 0) ? 0:1;

	return ret;
}
/* take dsi irq s32o account, todo? */
int disp_al_lcd_tri_start(u32 screen_id, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI)
	if (panel->lcd_if == LCD_IF_DSI)
		dsi_tri_start(screen_id);
#endif
	return tcon0_tri_start(screen_id);
}

int disp_al_lcd_io_cfg(u32 screen_id, u32 enable, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI)
	if (panel->lcd_if == LCD_IF_DSI) {
		if (enable) {
			dsi_io_open(screen_id, panel);
		} else {
			dsi_io_close(screen_id);
		}
	}
#endif

	return 0;
}

int disp_al_lcd_get_cur_line(u32 screen_id, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI)
		return dsi_get_cur_line(screen_id);
	else
#endif
	return tcon_get_cur_line(screen_id, 0);
}

int disp_al_lcd_get_start_delay(u32 screen_id, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI)
		return dsi_get_start_delay(screen_id);
	else
#endif
	return tcon_get_start_delay(screen_id, 0);
}

/* hdmi */
int disp_al_hdmi_enable(u32 screen_id)
{
	tcon1_open(screen_id);
	return 0;
}

int disp_al_hdmi_disable(u32 screen_id)
{
	al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_NONE;

	tcon1_close(screen_id);
	tcon_exit(screen_id);

	return 0;
}



int disp_al_hdmi_cfg(u32 screen_id, struct disp_video_timings *video_info)
{
	al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_HDMI;
	al_priv.output_mode[screen_id] = (u32)video_info->vic;
	al_priv.output_fps[screen_id] = video_info->pixel_clk / video_info->hor_total_time /\
		video_info->ver_total_time * (video_info->b_interlace + 1) / (video_info->trd_mode + 1);
	al_priv.tcon_index[screen_id] = 1;

	de_update_device_fps(al_priv.disp_disp[screen_id], al_priv.output_fps[screen_id]);

	tcon_init(screen_id);
	tcon1_set_timming(screen_id, video_info);
	if (al_priv.output_cs[screen_id] != 0)/* YUV output */
		tcon1_hdmi_color_remap(screen_id, 1);
	else
		tcon1_hdmi_color_remap(screen_id, 0);

	return 0;
}

/* tv */
int disp_al_tv_enable(u32 screen_id)
{
	tcon1_open(screen_id);
	return 0;
}

int disp_al_tv_disable(u32 screen_id)
{
	al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_NONE;

	tcon1_close(screen_id);
	tcon_exit(screen_id);

	return 0;
}

int disp_al_tv_cfg(u32 screen_id, struct disp_video_timings *video_info)
{
	al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_TV;
	al_priv.output_mode[screen_id] = (u32)video_info->tv_mode;
	al_priv.output_fps[screen_id] = video_info->pixel_clk / video_info->hor_total_time /\
		video_info->ver_total_time;
	al_priv.tcon_index[screen_id] = 1;

	de_update_device_fps(al_priv.disp_disp[screen_id], al_priv.output_fps[screen_id]);

	tcon_init(screen_id);
	tcon1_set_timming(screen_id, video_info);
	tcon1_yuv_range(screen_id, 1);

	return 0;
}

int disp_al_vdevice_cfg(u32 screen_id, struct disp_video_timings *video_info,
			struct disp_vdevice_interface_para *para,
			u8 config_tcon_only)
{
	struct lcd_clk_info clk_info;
	struct disp_panel_para info;

	if (para->sub_intf == LCD_HV_IF_CCIR656_2CYC) {
	printk("config tv mode\n");
		al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_TV;
	} else
		al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_LCD;
	al_priv.output_mode[screen_id] = (u32)para->intf;
	al_priv.output_fps[screen_id] = video_info->pixel_clk / video_info->hor_total_time /\
		video_info->ver_total_time;
	al_priv.tcon_index[screen_id] = 0;

	de_update_device_fps(al_priv.disp_disp[screen_id], al_priv.output_fps[screen_id]);

	memset(&info, 0, sizeof(struct disp_panel_para));
	info.lcd_if = para->intf;
	info.lcd_x = video_info->x_res;
	info.lcd_y = video_info->y_res;
	info.lcd_hv_if = (enum disp_lcd_hv_if)para->sub_intf;
	info.lcd_dclk_freq = video_info->pixel_clk;
	info.lcd_ht = video_info->hor_total_time;
	info.lcd_hbp = video_info->hor_back_porch + video_info->hor_sync_time;
	info.lcd_hspw = video_info->hor_sync_time;
	info.lcd_vt = video_info->ver_total_time;
	info.lcd_vbp = video_info->ver_back_porch + video_info->ver_sync_time;
	info.lcd_vspw = video_info->ver_sync_time;
	info.lcd_interlace = video_info->b_interlace;
	info.lcd_hv_syuv_fdly = para->fdelay;
	info.lcd_hv_clk_phase = para->clk_phase;
	info.lcd_hv_sync_polarity = para->sync_polarity;

	if (info.lcd_hv_if == LCD_HV_IF_CCIR656_2CYC)
		info.lcd_hv_syuv_seq = para->sequence;
	else
		info.lcd_hv_srgb_seq = para->sequence;
	tcon_init(screen_id);
	disp_al_lcd_get_clk_info(screen_id, &clk_info, &info);
	clk_info.tcon_div = 11;/* fixme */
	tcon0_set_dclk_div(screen_id, clk_info.tcon_div);
	if (info.lcd_hv_if == LCD_HV_IF_CCIR656_2CYC)
		tcon1_yuv_range(screen_id, 1);
	if (tcon0_cfg(screen_id, &info) != 0)
		DE_WRN("lcd cfg fail!\n");
	else
		DE_INF("lcd cfg ok!\n");

	return 0;
}

int disp_al_vdevice_enable(u32 screen_id)
{
	struct disp_panel_para panel;

	memset(&panel, 0, sizeof(struct disp_panel_para));
	panel.lcd_if = LCD_IF_HV;
	tcon0_open(screen_id, &panel);

	return 0;
}



int disp_al_vdevice_disable(u32 screen_id)
{
	al_priv.output_type[screen_id] = (u32)DISP_OUTPUT_TYPE_NONE;

	tcon0_close(screen_id);
	tcon_exit(screen_id);

	return 0;
}

/* screen_id: used for index of manager */
int disp_al_device_get_cur_line(u32 screen_id)
{
	u32 tcon_index = al_priv.tcon_index[screen_id];

	return tcon_get_cur_line(screen_id, tcon_index);
}

int disp_al_device_get_start_delay(u32 screen_id)
{
	u32 tcon_index = al_priv.tcon_index[screen_id];

	tcon_index = (al_priv.tcon_index[screen_id] == 0)?0:1;
	return tcon_get_start_delay(screen_id, tcon_index);
}

int disp_al_device_query_irq(u32 screen_id)
{
	int ret = 0;
	int irq_id = 0;

	irq_id = (al_priv.tcon_index[screen_id] == 0) ? \
LCD_IRQ_TCON0_VBLK:LCD_IRQ_TCON1_VBLK;
	ret = tcon_irq_query(screen_id, irq_id);

	return ret;
}

int disp_al_tv_irq_enable(u32 screen_id)
{
	tcon_irq_enable(screen_id, LCD_IRQ_TCON1_VBLK);

	return 0;
}

int disp_al_tv_irq_disable(u32 screen_id)
{
	tcon_irq_disable(screen_id, LCD_IRQ_TCON1_VBLK);

	return 0;
}


int disp_al_device_enable_irq(u32 screen_id)
{
	int ret = 0;
	int irq_id = 0;

	irq_id = (al_priv.tcon_index[screen_id] == 0) ? \
LCD_IRQ_TCON0_VBLK:LCD_IRQ_TCON1_VBLK;
	ret = tcon_irq_enable(screen_id, irq_id);

	return ret;
}

int disp_al_device_disable_irq(u32 screen_id)
{
	int ret = 0;
	int irq_id = 0;

	irq_id = (al_priv.tcon_index[screen_id] == 0) ? \
LCD_IRQ_TCON0_VBLK:LCD_IRQ_TCON1_VBLK;
	ret = tcon_irq_disable(screen_id, irq_id);

	return ret;
}

int disp_al_lcd_get_status(u32 screen_id, struct disp_panel_para *panel)
{

	return 0;
}


int disp_al_device_get_status(u32 screen_id)
{
	int ret = 0;

	ret = tcon_get_status(screen_id, al_priv.tcon_index[screen_id]);

	return ret;
}



int disp_init_al(struct disp_bsp_init_para *para)
{
	int i;

	memset(&al_priv, 0, sizeof(struct disp_al_private_data));
	de_al_init(para);
	de_enhance_init(para);
	de_ccsc_init(para);
	de_dcsc_init(para);
	WB_EBIOS_Init(para);
	de_clk_set_reg_base(para->reg_base[DISP_MOD_DE]);

	for (i = 0; i < DEVICE_NUM; i++) {
		tcon_set_reg_base(
		    i, para->reg_base[DISP_MOD_LCD0 + i]); /* calc lcd1 base */
	}

	for (i = 0; i < DE_NUM; i++) {
		if (de_feat_is_support_smbl(i))
			de_smbl_init(i, para->reg_base[DISP_MOD_DE]);
	}

	dsi_set_reg_base(0, para->reg_base[DISP_MOD_DSI0]);

	if (para->boot_info.sync == 1) {
		u32 disp = para->boot_info.disp;
		u32 chn, layer_id;
		u32 chn_num = de_feat_get_num_chns(disp);
		u32 layer_num;
		u32 logo_find = 0;
		u32 disp_device;
		struct disp_video_timings tt;

		memset(&tt, 0, sizeof(struct disp_video_timings));
		al_priv.disp_device[disp] = de_rtmx_get_mux(disp);
		disp_device = al_priv.disp_device[disp];

		/* should take care about this, extend display treated as a LCD OUTPUT */
		al_priv.output_type[disp_device] = para->boot_info.type;
		al_priv.output_mode[disp_device] = para->boot_info.mode;
		al_priv.tcon_index[disp_device] = 0;
#if defined(SUPPORT_HDMI)
		al_priv.tcon_index[disp_device] = (para->boot_info.type == DISP_OUTPUT_TYPE_HDMI)?1:al_priv.tcon_index[disp_device];
#endif
#if defined(SUPPORT_TV)
		al_priv.tcon_index[disp_device] = (para->boot_info.type == DISP_OUTPUT_TYPE_TV)?1:al_priv.tcon_index[disp_device];
#endif

		de_rtmx_sync_hw(disp);
		de_rtmx_get_display_size(disp, &al_priv.disp_size[disp].width, &al_priv.disp_size[disp].height);
		for (chn = 0; chn < chn_num; chn++) {
			layer_num = de_feat_get_num_layers_by_chn(disp, chn);
			for (layer_id = 0; layer_id < layer_num; layer_id++) {
				if (de_rtmx_get_lay_enabled(disp, chn, layer_id)) {
					logo_find = 1;
					break;
				}
			}
			if (logo_find)
				break;
		}

		if (logo_find) {
			struct disp_rect win;

			de_rtmx_get_lay_size(disp, chn, layer_id, al_priv.logo_info[disp].fb.size);
			de_rtmx_get_lay_address(disp, chn, layer_id, al_priv.logo_info[disp].fb.addr);
			de_rtmx_get_lay_win(disp, chn, layer_id, &win);
			al_priv.logo_info[disp].fb.format = de_rtmx_get_lay_format(disp, chn, layer_id);
			al_priv.logo_info[disp].fb.crop.width = ((long long)win.width)<<32;
			al_priv.logo_info[disp].fb.crop.height = ((long long)win.height)<<32;
		}
		al_priv.output_fps[disp_device] = 60;
		de_update_device_fps(disp, al_priv.output_fps[disp_device]);
	}

	return 0;
}


int disp_al_get_fb_info(unsigned int sel, struct disp_layer_info *info)
{
	memcpy(info, &al_priv.logo_info[sel], sizeof(struct disp_layer_info));
	return 0;
}


int disp_al_get_display_size(unsigned int screen_id, unsigned int *width, unsigned int *height)
{
	*width = al_priv.disp_size[screen_id].width;
	*height = al_priv.disp_size[screen_id].height;

	return 0;
}
void disp_al_show_builtin_patten(u32 hwdev_index, u32 patten)
{
	tcon_show_builtin_patten(hwdev_index, patten);
}
#endif
