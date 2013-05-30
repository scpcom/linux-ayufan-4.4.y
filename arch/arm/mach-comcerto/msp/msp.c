/*
 *  arch/arm/mach-comcerto/msp/msp.c
 *
 *  Copyright (C) 2004,2008,2012 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <asm/fiq.h>
#include <linux/platform_device.h>

#include "ved.h"
#include "msp.h"

#define MSP_READY_DELAY (8000UL) /* msec */
#define MSP_PROC_SECTION "msp_proc"
#define VOIP_ENTRY_SECTION "voip_entries"

#define MAGIC_LEN (0x24)
#define MAGIC_OFF (0x20)

#define MAX_SAVED_ALERT (20)

#define COMCERTO_ATTR_SHOW(_name)	  \
	static ssize_t comcerto_show_##_name(struct device *dev, struct device_attribute *attr, char *buf); \
	static DEVICE_ATTR(_name, 0444, comcerto_show_##_name, NULL)

#define COMCERTO_ATTR_SET(_name)	  \
	static ssize_t comcerto_set_##_name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count); \
	static DEVICE_ATTR(_name, 0200, NULL, comcerto_set_##_name)

#define COMCERTO_ATTR(_name)	  \
	static ssize_t comcerto_show_##_name(struct device *dev, struct device_attribute *attr, char *buf); \
	static ssize_t comcerto_set_##_name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count); \
	static DEVICE_ATTR(_name, 0644, comcerto_show_##_name, comcerto_set_##_name)

COMCERTO_ATTR_SHOW(msp_info);
COMCERTO_ATTR_SHOW(alert_info);
COMCERTO_ATTR_SHOW(abi_rev);

struct voip_fiq_code {
	u32 len;
	u8 data[MAGIC_LEN];	   /* use hardcoded instruction, see below */
};

static int voip_fiq_op(void *p, int release);

static struct fiq_handler voip_fiq_handler = {
	.name = "Comcerto VoIPoFIQ",
	.fiq_op = voip_fiq_op,
	.dev_id = NULL
};

static void (*voip_entry)(void); /* VoIP code entry point */
static void (*voip_exit)(void);  /* VoIP code exit point */

static void msp_poll(unsigned long arg);
void update_pfe_status_for_MSP(void);
static volatile struct msp_info * global_msp_info;

struct pfe_info {
	unsigned long buf_baseaddr;
	unsigned long cbus_baseaddr;
	void *owner;
};

struct tpfe_status {
	unsigned long pfe_state;
	unsigned long pfe_virt;
	unsigned long axi_virt;
	void *owner;
} pfe_status;

/* PFE inform MSP it is started and its parameters*/
int msp_register_pfe(struct pfe_info *pfe_sync_info)
{
	/* save PFE status */
	pfe_status.pfe_state = 1;
	pfe_status.pfe_virt = pfe_sync_info->buf_baseaddr;
	pfe_status.axi_virt = pfe_sync_info->cbus_baseaddr;
	pfe_status.owner = pfe_sync_info->owner;

	if (global_msp_info) {
		/* msp is running, lock PFE, update PFE shared memory for MSP */
		try_module_get(pfe_status.owner);
		update_pfe_status_for_MSP();
	}

	return 0;
}
EXPORT_SYMBOL(msp_register_pfe);

void update_pfe_status_for_MSP(void)
{
	global_msp_info->pfe_ready = pfe_status.pfe_state;
	global_msp_info->pfe_virt = pfe_status.pfe_virt;
	global_msp_info->pfe_phys = COMCERTO_PFE_DDR_BASE;
	global_msp_info->axi_virt = pfe_status.axi_virt;
	global_msp_info->axi_phys = COMCERTO_AXI_EXP_BASE;
	global_msp_info->msp_virt = COMCERTO_MSP_VADDR;
	global_msp_info->msp_phys = COMCERTO_MSP_DDR_BASE;
}


/* PFE inform MSP it is stopped */
void msp_unregister_pfe(void)
{
	/* save PFE status */
	pfe_status.pfe_state = 0;

	if (global_msp_info) {
		update_pfe_status_for_MSP();
	}
}
EXPORT_SYMBOL(msp_unregister_pfe);

static int voip_fiq_op(void *p, int release)
{
	return 0;
}

/**
 * load_elf
 *
 *
 */
static int load_elf(struct _code_info *code_info)
{
	Elf32_Ehdr *this_elf_header = (Elf32_Ehdr *)(code_info->code);
	Elf32_Half number_of_section = this_elf_header->e_shnum;
	/* pointer to the section header */
	Elf32_Shdr *this_section_headers = (Elf32_Shdr *)(code_info->code + this_elf_header->e_shoff);
	Elf32_Off string_section_offset = this_section_headers[this_elf_header->e_shstrndx].sh_offset;
	const char *section_name = NULL;
	int rc = 0;
	int i = 0;

	if (!number_of_section) {
		printk(KERN_ERR "error loading elf: number of section is zero\n");
		rc = -1;

		goto out;
	}

	/* parse all sections */
	for (i = 0; i < number_of_section; i++) {
		section_name = code_info->code  + string_section_offset + this_section_headers[i].sh_name;

		if (!strncmp(section_name, MSP_PROC_SECTION, strlen(MSP_PROC_SECTION))) {
			code_info->proc_addr = this_section_headers[i].sh_addr;
		}

		if (!strncmp(section_name, VOIP_ENTRY_SECTION, strlen(VOIP_ENTRY_SECTION))) {
			code_info->sym_addr = this_section_headers[i].sh_addr;
		}

		/* retrieve the section name from the ELF buffer */

		if ((this_section_headers[i].sh_flags != SHF_MIPS_ADDR)
		    && this_section_headers[i].sh_flags
			&& strncmp(section_name, "CHECKSUM", 8))
		{
			/* retrieve the section name from the ELF buffer */

			if ((this_section_headers[i].sh_type & 3) == SHT_PROGBITS) {
				memcpy(
					(void *)this_section_headers[i].sh_addr,
					(void*)(code_info->code + this_section_headers[i].sh_offset),
					this_section_headers[i].sh_size);
			}
		}
	}

	if (code_info->proc_addr && code_info->sym_addr) {
		code_info->program_addr = this_elf_header->e_entry;
	} else {
		printk(KERN_ERR "error loading elf: could not find %s or %s\n", MSP_PROC_SECTION, VOIP_ENTRY_SECTION);

		rc = -1;
	}

out:
	return rc;
}

static int msp_ready(struct comcerto_msp *msp, unsigned long timeout)
{
	/* timeout is passed in ms, set it in jiffies */
	timeout = jiffies + (timeout * HZ) / 1000;

	while (!msp->info || !msp->info->ready) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(1);

		if (time_after(jiffies, timeout) && (!msp->info || !msp->info->ready)) {
			printk(KERN_ERR "error: could not receive ack from VoIP\n");
			msp->info = NULL;

			goto err;
		}
	}

	return 1;


err:
	return 0;
}


static int msp_start(struct comcerto_msp *msp)
{
	msp->state = MSP_STARTING;
	msp->alert_seen = 0;

	if (!msp_ready(msp, MSP_READY_DELAY)) {
		printk(KERN_ERR "VoIP starting failed\n");

		goto err;
	}

	printk(KERN_INFO "VoIP has been started successfully\n");

	/* start heart beat timer */
	init_timer(&msp->timer_expire);
	msp->timer_expire.function = msp_poll;
	msp->timer_expire.expires = jiffies + 1 * HZ; /* 1sec */
	msp->timer_expire.data = (unsigned long)msp;
	add_timer(&msp->timer_expire);
	msp->state = MSP_RUNNING;
	global_msp_info = msp->info;

	if (pfe_status.pfe_state == 1) {
		try_module_get(pfe_status.owner);
		update_pfe_status_for_MSP();
	}

	return 0;


err:
	return -1;
}

static void msp_alert(struct comcerto_msp *msp)
{
	struct alert_type *alert = NULL;
	int ialert = 0;

	if (!msp->info) {
		return;
	}

	while (msp->info->alert_number > msp->alert_seen) {
		if (msp->alert_seen >= MAX_SAVED_ALERT) {
			ialert = MAX_SAVED_ALERT - 1;
		} else {
			ialert = msp->alert_seen;
		}

		alert = (struct alert_type *)(msp->info->save_alert) + ialert;
		msp->alert_seen++;

		printk(KERN_ERR "VoIP alert: current number / total received = %lu/%lu\n", msp->alert_seen, msp->info->alert_number);
		printk(KERN_ERR "\ttype: 0x%02x\n", alert->type & 0xff);
		printk(KERN_ERR "\tchannel: %hu\n", alert->channel);
		printk(KERN_ERR "\tlink register: 0x%08x\n", alert->abort_lr);
		printk(KERN_ERR "\tunique ID: 0x%04x\n", alert->unique_id);
		printk(KERN_ERR "\taction: 0x%04x\n", alert->action);
		printk(KERN_ERR "\tlocaltime: 0x%08x\n", alert->localtime);
		printk(KERN_ERR "\tval1: 0x%08x\n", alert->val1);
		printk(KERN_ERR "\tval2: 0x%08x\n", alert->val2);
	}
}

static void msp_poll(unsigned long arg)
{
	struct comcerto_msp *msp = (struct comcerto_msp *)arg;

	switch (msp->state) {
		case MSP_RESETTING:
		case MSP_RESET:
		case MSP_STARTING:
		case MSP_CRASHED:
			break;
		case MSP_RUNNING: {
			/* check for alerts */
			msp_alert(msp);

			if (!msp->info->heartbeat) {
				printk(KERN_ERR "VoIP heart beat failure\n");
				msp->state = MSP_CRASHED;
			}

			msp->info->heartbeat = 0;
		} break;
	}

	msp->timer_expire.expires = jiffies + 1 * HZ; /* 1sec */
	add_timer(&msp->timer_expire);
}

int comcerto_download_to_msp(struct comcerto_msp *msp)
{
	struct _code_info *code_info = &msp->code_info;
	struct voip_sym *voip_sym;
	struct voip_fiq_code voip_fiq;
	void (*rtxc_handler)(void); /* FIQ handler code entry point */
	int rc = 0;

	if (load_elf(code_info)) {
		printk(KERN_ERR "VoIP download failed\n");
		rc = -EINVAL;

		goto err;
	}

	/* save msp info pointer */
	msp->info = (struct msp_info *)msp->code_info.proc_addr;
	voip_sym = (struct voip_sym *)msp->code_info.sym_addr;

	rtxc_handler = voip_sym->rtxc_handler;
	voip_entry = voip_sym->voip_entry;
	voip_exit = voip_sym->voip_exit;

	if (!voip_entry || !voip_exit || !rtxc_handler) {
		printk(KERN_ERR "failed extract symbol\n");
		rc = -EINVAL;

		goto err;
	}

	if (claim_fiq(&voip_fiq_handler)) {
		printk(KERN_ERR "could not claim FIQ\n");
		rc = -EINVAL;

		goto err;
	} else {
		/* copy code to IVT */
		unsigned int code = 0xe59ff018; /* LDR pc, [pc, #0x20]; here 0x20 == MAGIC_OFF, do not change it */

		voip_fiq.len = MAGIC_LEN;
		memcpy(voip_fiq.data + MAGIC_OFF, &rtxc_handler, sizeof(rtxc_handler));
		memcpy(voip_fiq.data, &code, sizeof(code));

		/* copy code to IVT */
		set_fiq_handler(&voip_fiq.data[0], voip_fiq.len);
	}

err:
	return rc;
}

int comcerto_start_msp(struct comcerto_msp *msp)
{

#ifdef CONFIG_SMP
	struct cpumask in_mask;
	int cpu = 0;
	int pid = 0;

	/* start voip on cpu where FIQ handler is registered */
	cpumask_set_cpu(cpu, &in_mask);
	sched_setaffinity(pid, &in_mask);
#endif  /* CONFIG_SMP */

	/* give control to MSP */
	voip_entry();

	return msp_start(msp);
}

void comcerto_stop_msp(struct comcerto_msp *msp)
{
	/* stop MSP gently */
	voip_exit();

	printk(KERN_INFO "VoIP has been stopped\n");

	release_fiq(&voip_fiq_handler);

	msp->state = MSP_RESET;
	del_timer(&msp->timer_expire);
	msp->info = NULL;

	if (global_msp_info && (pfe_status.pfe_state == 1)) {
		module_put(pfe_status.owner);
	}

	global_msp_info = 0;
}

static ssize_t comcerto_show_msp_info(struct device *dev,
                                      struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ved_priv *priv = netdev_priv(ndev);
	struct comcerto_msp *msp = &priv->msp;
	int len = 0;

	if (!msp->info) {
		return sprintf(buf + len, "no VoIP info available\n");
	}

	len += sprintf(buf + len, "ABI version: %lu\n", msp->info->abi_rev);
	len += sprintf(buf + len, "VoIP version: %s\n", msp->info->msp_version);
	len += sprintf(buf + len, "DSP version: %s\n", msp->info->spu_version);
	len += sprintf(buf + len, "VoIP freq: %d Mhz\n", msp->info->ARMfreq);
	len += sprintf(buf + len, "AMBA bus freq: %d MHz\n", msp->info->AMBAfreq);
	len += sprintf(buf + len, "Alert number: %lu\n", msp->alert_seen);

	return len;
}

static ssize_t comcerto_show_alert_info(struct device *dev,
                                        struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ved_priv *priv = netdev_priv(ndev);
	struct comcerto_msp *msp = &priv->msp;
	struct alert_type *alert;
	unsigned long ialert = 0;
	int len = 0;

	if ( !msp->info || !msp->alert_seen ) {
		return sprintf(buf + len, "no VoIP alert info available\n");
	}

	if (msp->alert_seen > MAX_SAVED_ALERT) {
		ialert = MAX_SAVED_ALERT;
	} else {
		ialert = msp->alert_seen;
	}

	for (; ialert > 0; ialert--) {
		alert = (struct alert_type *)(msp->info->save_alert) + ialert - 1;

		len += sprintf(buf + len, "alert No: %lu\n", ((ialert == MAX_SAVED_ALERT) ? msp->alert_seen : ialert));
		len += sprintf(buf + len, "\ttype: 0x%02X\n", (alert->type & 0xff));
		len += sprintf(buf + len, "\tchannel: %hu\n", alert->channel);
		len += sprintf(buf + len, "\tunique ID: 0x%04X\n", alert->unique_id);
		len += sprintf(buf + len, "\taction: 0x%04X\n", alert->action);
		len += sprintf(buf + len, "\tlink register: 0x%08X\n", alert->abort_lr);
		len += sprintf(buf + len, "\tlocaltime: 0x%08X\n", alert->localtime);
		len += sprintf(buf + len, "\tval1: 0x%08X\n", alert->val1);
		len += sprintf(buf + len, "\tval2: 0x%08X\n", alert->val2);

		/* Make sure we are not going out of buffer. Number to add is
		   empirical, so, change buf above - change number below */
		if (len + 175 > PAGE_SIZE) {
			break;
		}
	}

	return len;
}

static ssize_t comcerto_show_abi_rev(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{

	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct ved_priv *priv = netdev_priv(ndev);
	struct comcerto_msp *msp = &priv->msp;
	int len = 0;

	if (!msp->info) {
		return sprintf(buf + len, "no VoIP info available\n");
	}

	len += sprintf(buf + len, "%lu\n", msp->info->abi_rev);

	return len;
}

int msp_init_sysfs(struct platform_device *pdev)
{
	if (device_create_file(&pdev->dev, &dev_attr_msp_info)
	    || device_create_file(&pdev->dev, &dev_attr_alert_info)
	    || device_create_file(&pdev->dev, &dev_attr_abi_rev))
	{
		printk(KERN_ERR "failed to create VoIP sysfs files\n");

		return -1;
	}

	return 0;
}
