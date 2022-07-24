/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN55IW3_DSP_CCU_H_
#define _DT_BINDINGS_CLK_SUN55IW3_DSP_CCU_H_

#define CLK_PLL_AUDIO1		0
#define CLK_PLL_AUDIO1_DIV2	1
#define CLK_PLL_AUDIO1_DIV5	2
#define CLK_PLL_AUDIO_OUT	3
#define CLK_DSP_DSP		4
#define CLK_DSP_AHB		5
#define CLK_APB			6
#define CLK_I2S0		7
#define CLK_I2S1		8
#define CLK_I2S2		9
#define CLK_I2S3		10
#define CLK_I2S3_ASRC		11
#define CLK_I2S0_BGR		12
#define CLK_I2S1_BGR		13
#define CLK_I2S2_BGR		14
#define CLK_I2S3_BGR		15
#define CLK_SPDIF_TX		16
#define CLK_SPDIF_RX		17
#define CLK_BUS_SPDIF		18
#define CLK_DMIC		19
#define CLK_DMIC_BUS		20
#define CLK_AUDIO_CODEC_DAC	21
#define CLK_AUDIO_CODEC_ADC	22
#define CLK_AUDIO_CODEC		23
#define CLK_DSP_MSG		24
#define CLK_DSP_CFG		25
#define CLK_NPU_ACLK		26
#define CLK_NPU_HCLK		27
#define CLK_DSP_NPU		28
#define CLK_DSP_TIMER0		29
#define CLK_DSP_TIMER1		30
#define CLK_DSP_TIMER2		31
#define CLK_DSP_TIMER3		32
#define CLK_DSP_TIMER4		33
#define CLK_DSP_TIMER5		34
#define CLK_BUS_DSP_TIMER	35
#define CLK_DSP_DMA		36
#define CLK_TZMA0		37
#define CLK_TZMA1		38
#define CLK_PUBSRAM		39
#define CLK_DSP_MBUS		40
#define CLK_DMA_MBUS		41
#define CLK_RV			42
#define CLK_RV_CFG		43
#define CLK_RISCV_MSG		44
#define CLK_DSP_PWM		45
#define CLK_PWM_BGR		46
#define CLK_AHB_AUTO		47

#define CLK_DSP_NUMBER		(CLK_AHB_AUTO + 1)

#endif /* _DT_BINDINGS_CLK_SUN55IW3_DSP_CCU_H_ */
