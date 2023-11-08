/*
 * CDB emulation for non-READ/WRITE commands.
 *
 * Copyright (c) 2002, 2003, 2004, 2005 PyX Technologies, Inc.
 * Copyright (c) 2005, 2006, 2007 SBE, Inc.
 * Copyright (c) 2007-2010 Rising Tide Systems
 * Copyright (c) 2008-2010 Linux-iSCSI.org
 *
 * Nicholas A. Bellinger <nab@kernel.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <scsi/scsi.h>

#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>

#include "target_core_internal.h"
#include "target_core_ua.h"

#if defined(CONFIG_MACH_QNAPTS)
#include "target_core_iblock.h"
#include "target_core_file.h"
#include "vaai_target_struc.h"
#include "target_general.h"

#if defined(SUPPORT_VAAI) //Benjamin 20121105 sync VAAI support from Adam. 
#include "vaai_helper.h"
#endif
#if defined(SUPPORT_TPC_CMD)
#include "tpc_helper.h"
#endif
#if defined(SUPPORT_FAST_BLOCK_CLONE)
#include "target_fast_clone.h"
#endif

#if defined(SUPPORT_TP)
#include "tp_def.h"
extern int vaai_logsense_lbp(IN LIO_SE_CMD *pSeCmd, IN u8 *pu8Buff);
extern int vaai_modesense_lbp(IN LIO_SE_CMD *pCmd, IN u8 *pu8Buff);
extern void vaai_build_pg_desc(IN LIO_SE_CMD *pCmd, IN u8 *pu8buf);

#endif

////////////////////////////////////////////////////
//
// to declare the global data here ...
//
////////////////////////////////////////////////////

/* log sense command function table */
LOGSENSE_FUNC_TABLE g_logsense_table[] ={
#if defined(SUPPORT_TP)
    {0xc, 0x0, vaai_logsense_lbp, 0x0},
#endif
    {0x0, 0x0, NULL, 0x1},
};

/* The settings may be used by WRITE SAME or UNMAP commands */
u8 bSupportArchorLba = SUPPORT_ANCH_LBA;
u8 bReturnZeroWhenReadUnmapLba = RET_ZERO_READ_UNMAP_LAB;  /* Block provisioning setting */


#endif /* defined(CONFIG_MACH_QNAPTS) */

static void
target_fill_alua_data(struct se_port *port, unsigned char *buf)
{
	struct t10_alua_tg_pt_gp *tg_pt_gp;
	struct t10_alua_tg_pt_gp_member *tg_pt_gp_mem;

	/*
	 * Set SCCS for MAINTENANCE_IN + REPORT_TARGET_PORT_GROUPS.
	 */
	buf[5]	= 0x80;

	/*
	 * Set TPGS field for explict and/or implict ALUA access type
	 * and opteration.
	 *
	 * See spc4r17 section 6.4.2 Table 135
	 */
	if (!port)
		return;
	tg_pt_gp_mem = port->sep_alua_tg_pt_gp_mem;
	if (!tg_pt_gp_mem)
		return;

	spin_lock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
	tg_pt_gp = tg_pt_gp_mem->tg_pt_gp;
	if (tg_pt_gp)
		buf[5] |= tg_pt_gp->tg_pt_gp_alua_access_type;
	spin_unlock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
}

static int
target_emulate_inquiry_std(struct se_cmd *cmd, char *buf)
{
	struct se_lun *lun = cmd->se_lun;
	struct se_device *dev = cmd->se_dev;
#ifdef CONFIG_MACH_QNAPTS // Benjamin 20110309: Shall return spaces(0x20) instead of zeros in ASCII field.
    int i;
#endif 

	/* Set RMB (removable media) for tape devices */
	if (dev->transport->get_device_type(dev) == TYPE_TAPE)
		buf[1] = 0x80;

	buf[2] = dev->transport->get_device_rev(dev);

	/*
	 * NORMACA and HISUP = 0, RESPONSE DATA FORMAT = 2
	 *
	 * SPC4 says:
	 *   A RESPONSE DATA FORMAT field set to 2h indicates that the
	 *   standard INQUIRY data is in the format defined in this
	 *   standard. Response data format values less than 2h are
	 *   obsolete. Response data format values greater than 2h are
	 *   reserved.
	 */
	buf[3] = 2;

	/*
	 * Enable SCCS and TPGS fields for Emulated ALUA
	 */
	if (dev->se_sub_dev->t10_alua.alua_type == SPC3_ALUA_EMULATED)
		target_fill_alua_data(lun->lun_sep, buf);

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_VAAI) //Benjamin 20121105 sync VAAI support from Adam. 
	buf[5] |= 0x8; /* 3PC bit, to report support extended copy command */
#endif
	buf[7] = 0x2; /* CmdQue=1 */

#ifdef CONFIG_MACH_QNAPTS // 2009/7/31 Nike Chen change ID to QNAP 
#if defined(Athens)
	snprintf(&buf[8], 6, "Cisco");
#elif defined(IS_G)
	snprintf(&buf[8], 4, "NAS");
#else
	snprintf(&buf[8], 5, "QNAP");
#endif /* #if defined(Athens) */
#else
	snprintf(&buf[8], 8, "LIO-ORG");
#endif /* #ifdef CONFIG_MACH_QNAPTS  */
	snprintf(&buf[16], 16, "%s", dev->se_sub_dev->t10_wwn.model);
	snprintf(&buf[32], 4, "%s", dev->se_sub_dev->t10_wwn.revision);
#ifdef CONFIG_MACH_QNAPTS 
    //  Benjamin 20130111: Version Descriptor should at least contains iSCSI (0960h). See SPC-4 section 6.4.2 Table 144
    buf[58] = 0x09;
    buf[59] = 0x60;
    // Benjamin 20110309: Shall return spaces(0x20) instead of zeros in ASCII field.
    for(i = 8; i < cmd->data_length; i++)
        buf[i] = buf[i] == '\0' ? 0x20 : buf[i];
    
	buf[4] = 55; /* Set additional length to 55 */    
#else
	buf[4] = 31; /* Set additional length to 31 */
#endif    

	return 0;
}

/* unit serial number */
static int
target_emulate_evpd_80(struct se_cmd *cmd, unsigned char *buf)
{
	struct se_device *dev = cmd->se_dev;
	u16 len = 0;

	if (dev->se_sub_dev->su_dev_flags &
			SDF_EMULATED_VPD_UNIT_SERIAL) {
		u32 unit_serial_len;

		unit_serial_len = strlen(dev->se_sub_dev->t10_wwn.unit_serial);
		unit_serial_len++; /* For NULL Terminator */

		len += sprintf(&buf[4], "%s",
			dev->se_sub_dev->t10_wwn.unit_serial);
#ifdef CONFIG_MACH_QNAPTS 
		/*
		 * Benjamin 20130115: 
		 * First, NULL Terminator should be skipped for text response.
		 * Second, Make the Page Length be consistent with DataSegmentLength.
		 */
		cmd->data_length = (cmd->data_length - 4) > len ? len + 4 : cmd->data_length;        
#else    
		len++; /* Extra Byte for NULL Terminator */
#endif
		buf[3] = len;
	}

	return 0;
}

#ifdef CONFIG_MACH_QNAPTS // adam hsu for BUG 29894: copy from target module in kernel 2.6.33.2
unsigned char transport_asciihex_to_binaryhex(
    unsigned char val[2]
    )
{
	unsigned char result = 0;
	/*
	 * MSB
	 */
	if ((val[0] >= 'a') && (val[0] <= 'f'))
		result = ((val[0] - 'a' + 10) & 0xf) << 4;
	else
		if ((val[0] >= 'A') && (val[0] <= 'F'))
			result = ((val[0] - 'A' + 10) & 0xf) << 4;
		else /* digit */
			result = ((val[0] - '0') & 0xf) << 4;
	/*
	 * LSB
	 */
	if ((val[1] >= 'a') && (val[1] <= 'f'))
		result |= ((val[1] - 'a' + 10) & 0xf);
	else
		if ((val[1] >= 'A') && (val[1] <= 'F'))
			result |= ((val[1] - 'A' + 10) & 0xf);
		else /* digit */
			result |= ((val[1] - '0') & 0xf);

	return result;
}
#endif 

static void
target_parse_naa_6h_vendor_specific(struct se_device *dev, unsigned char *buf)
{
#ifdef CONFIG_MACH_QNAPTS
    /* 
        * Benjamin 20120117: NAA naming is a persistent naming for VMWare, so we cannot change it.
        * Otherwise the lun created with the old iscsi driver will not be recognized by VMWare 
        * after updating to the new iscsi lio driver.
        * For  BUG 29894,  we need to use the naming from target module in kernel 2.6.33.2;
        * For X69 3.8.1, we need to use the naming from target module in kernel 3.4.6.
        */
    if(likely(strcmp(dev->se_sub_dev->se_dev_naa, "3.8.1"))) {        
        u8 binary = 0, binary_new =0 , off= 0, i = 0;

        pr_debug("%s:NAA vendor specific with the old method.\n", __func__);                
        binary = transport_asciihex_to_binaryhex(&dev->se_sub_dev->t10_wwn.unit_serial[0]);
        
        buf[off++] |= (binary & 0xf0) >> 4;

        for (i = 0; i < 24; i += 2) {
            binary_new = transport_asciihex_to_binaryhex(
            &dev->se_sub_dev->t10_wwn.unit_serial[i+2]);
            buf[off] = (binary & 0x0f) << 4;
            buf[off++] |= (binary_new & 0xf0) >> 4;
            binary = binary_new;
        }
    }
    else {

    	unsigned char *p = &dev->se_sub_dev->t10_wwn.unit_serial[0];
    	int cnt;
    	bool next = true;

		pr_err("%s:NAA vendor specific with the new method.\n", __func__);                
		/*
		 * Generate up to 36 bits of VENDOR SPECIFIC IDENTIFIER starting on
		 * byte 3 bit 3-0 for NAA IEEE Registered Extended DESIGNATOR field
		 * format, followed by 64 bits of VENDOR SPECIFIC IDENTIFIER EXTENSION
		 * to complete the payload.  These are based from VPD=0x80 PRODUCT SERIAL
		 * NUMBER set via vpd_unit_serial in target_core_configfs.c to ensure
		 * per device uniqeness.
		 */
    	for (cnt = 0; *p && cnt < 13; p++) {
    		int val = hex_to_bin(*p);

    		if (val < 0)
    			continue;

    		if (next) {
    			next = false;
    			buf[cnt++] |= val;
    		} else {
    			next = true;
    			buf[cnt] = val << 4;
    		}
    	}        
    }
#else   /* #ifdef CONFIG_MACH_QNAPTS */

	unsigned char *p = &dev->se_sub_dev->t10_wwn.unit_serial[0];
	int cnt;
	bool next = true;

	/*
	 * Generate up to 36 bits of VENDOR SPECIFIC IDENTIFIER starting on
	 * byte 3 bit 3-0 for NAA IEEE Registered Extended DESIGNATOR field
	 * format, followed by 64 bits of VENDOR SPECIFIC IDENTIFIER EXTENSION
	 * to complete the payload.  These are based from VPD=0x80 PRODUCT SERIAL
	 * NUMBER set via vpd_unit_serial in target_core_configfs.c to ensure
	 * per device uniqeness.
	 */
	for (cnt = 0; *p && cnt < 13; p++) {
		int val = hex_to_bin(*p);

		if (val < 0)
			continue;

		if (next) {
			next = false;
			buf[cnt++] |= val;
		} else {
			next = true;
			buf[cnt] = val << 4;
		}
	}
#endif  /* #ifdef CONFIG_MACH_QNAPTS */
}

/*
 * Device identification VPD, for a complete list of
 * DESIGNATOR TYPEs see spc4r17 Table 459.
 */
static int
target_emulate_evpd_83(struct se_cmd *cmd, unsigned char *buf)
{
	struct se_device *dev = cmd->se_dev;
	struct se_lun *lun = cmd->se_lun;
	struct se_port *port = NULL;
	struct se_portal_group *tpg = NULL;
	struct t10_alua_lu_gp_member *lu_gp_mem;
	struct t10_alua_tg_pt_gp *tg_pt_gp;
	struct t10_alua_tg_pt_gp_member *tg_pt_gp_mem;
	unsigned char *prod = &dev->se_sub_dev->t10_wwn.model[0];
	u32 prod_len;
	u32 unit_serial_len, off = 0;
	u16 len = 0, id_len;

	off = 4;

	/*
	 * NAA IEEE Registered Extended Assigned designator format, see
	 * spc4r17 section 7.7.3.6.5
	 *
	 * We depend upon a target_core_mod/ConfigFS provided
	 * /sys/kernel/config/target/core/$HBA/$DEV/wwn/vpd_unit_serial
	 * value in order to return the NAA id.
	 */
	if (!(dev->se_sub_dev->su_dev_flags & SDF_EMULATED_VPD_UNIT_SERIAL))
		goto check_t10_vend_desc;

	/* CODE SET == Binary */
	buf[off++] = 0x1;

	/* Set ASSOCIATION == addressed logical unit: 0)b */
	buf[off] = 0x00;

	/* Identifier/Designator type == NAA identifier */
	buf[off++] |= 0x3;
	off++;

	/* Identifier/Designator length */
	buf[off++] = 0x10;

#ifdef CONFIG_MACH_QNAPTS
	/* 
	 * Here is very important and please take care it. For vmware's VAAI requirement,
	 * the xcopy function will use the Identification Descriptor CSCD descriptor format
	 * and the values in fields of DESIGNATOR TYPE and DESIGNATOR will depend on the
	 * vpd 83h data. In other words, if you will change their values, please
	 * also take care something in our xcopy function.
	 */ 
    if(!strcmp(dev->se_sub_dev->se_dev_naa, "qnap")) {
		pr_debug("%s:NAA with QNAP IEEE company ID.\n", __func__);        
        /* This code is for new firmware version (3.9.0) */

        /* Start NAA IEEE Registered Extended Identifier/Designator */
        buf[off++] = (0x6 << 4)| 0x0e;
        /* Use QNAP IEEE Company ID: */
        buf[off++] = 0x84;
        buf[off++] = 0x3b;
        buf[off] = (0x6 << 4);
    }        
    else {
		pr_debug("%s:NAA with OpenFabrics IEEE company ID.\n", __func__);                
		/*
		 * Start NAA IEEE Registered Extended Identifier/Designator
		 */
		buf[off++] = (0x6 << 4);

		/*
		 * Use OpenFabrics IEEE Company ID: 00 14 05
		 */
		buf[off++] = 0x01;
		buf[off++] = 0x40;
		buf[off] = (0x5 << 4);
    }
#else /* #ifdef CONFIG_MACH_QNAPTS */
	/*
	 * Start NAA IEEE Registered Extended Identifier/Designator
	 */
	buf[off++] = (0x6 << 4);

	/*
	 * Use OpenFabrics IEEE Company ID: 00 14 05
	 */
	buf[off++] = 0x01;
	buf[off++] = 0x40;
	buf[off] = (0x5 << 4);
#endif /* #ifdef CONFIG_MACH_QNAPTS */

	/*
	 * Return ConfigFS Unit Serial Number information for
	 * VENDOR_SPECIFIC_IDENTIFIER and
	 * VENDOR_SPECIFIC_IDENTIFIER_EXTENTION
	 */
	target_parse_naa_6h_vendor_specific(dev, &buf[off]);

	len = 20;
	off = (len + 4);

check_t10_vend_desc:
	/*
	 * T10 Vendor Identifier Page, see spc4r17 section 7.7.3.4
	 */
	id_len = 8; /* For Vendor field */
	prod_len = 4; /* For VPD Header */
	prod_len += 8; /* For Vendor field */
	prod_len += strlen(prod);
	prod_len++; /* For : */

	if (dev->se_sub_dev->su_dev_flags &
			SDF_EMULATED_VPD_UNIT_SERIAL) {
		unit_serial_len =
			strlen(&dev->se_sub_dev->t10_wwn.unit_serial[0]);
		unit_serial_len++; /* For NULL Terminator */

		id_len += sprintf(&buf[off+12], "%s:%s", prod,
				&dev->se_sub_dev->t10_wwn.unit_serial[0]);
	}
	buf[off] = 0x2; /* ASCII */
	buf[off+1] = 0x1; /* T10 Vendor ID */
	buf[off+2] = 0x0;
#ifdef CONFIG_MACH_QNAPTS // 2009/7/31 Nike Chen change ID to QNAP
#if defined(Athens)
	memcpy(&buf[off+4], "Cisco", 6);
#elif defined(IS_G)
	memcpy(&buf[off+4], "NAS", 4);
#else
	memcpy(&buf[off+4], "QNAP", 5);
#endif /* #if defined(Athens) */
#else
	memcpy(&buf[off+4], "LIO-ORG", 8);
#endif /* #ifdef CONFIG_MACH_QNAPTS */    
	/* Extra Byte for NULL Terminator */
	id_len++;
	/* Identifier Length */
	buf[off+3] = id_len;
	/* Header size for Designation descriptor */
	len += (id_len + 4);
	off += (id_len + 4);
	/*
	 * struct se_port is only set for INQUIRY VPD=1 through $FABRIC_MOD
	 */
	port = lun->lun_sep;
	if (port) {
		struct t10_alua_lu_gp *lu_gp;
		u32 padding, scsi_name_len;
		u16 lu_gp_id = 0;
		u16 tg_pt_gp_id = 0;
		u16 tpgt;

		tpg = port->sep_tpg;
		/*
		 * Relative target port identifer, see spc4r17
		 * section 7.7.3.7
		 *
		 * Get the PROTOCOL IDENTIFIER as defined by spc4r17
		 * section 7.5.1 Table 362
		 */
		buf[off] =
			(tpg->se_tpg_tfo->get_fabric_proto_ident(tpg) << 4);
		buf[off++] |= 0x1; /* CODE SET == Binary */
		buf[off] = 0x80; /* Set PIV=1 */
		/* Set ASSOCIATION == target port: 01b */
		buf[off] |= 0x10;
		/* DESIGNATOR TYPE == Relative target port identifer */
		buf[off++] |= 0x4;
		off++; /* Skip over Reserved */
		buf[off++] = 4; /* DESIGNATOR LENGTH */
		/* Skip over Obsolete field in RTPI payload
		 * in Table 472 */
		off += 2;
		buf[off++] = ((port->sep_rtpi >> 8) & 0xff);
		buf[off++] = (port->sep_rtpi & 0xff);
		len += 8; /* Header size + Designation descriptor */
		/*
		 * Target port group identifier, see spc4r17
		 * section 7.7.3.8
		 *
		 * Get the PROTOCOL IDENTIFIER as defined by spc4r17
		 * section 7.5.1 Table 362
		 */
		if (dev->se_sub_dev->t10_alua.alua_type !=
				SPC3_ALUA_EMULATED)
			goto check_scsi_name;

		tg_pt_gp_mem = port->sep_alua_tg_pt_gp_mem;
		if (!tg_pt_gp_mem)
			goto check_lu_gp;

		spin_lock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
		tg_pt_gp = tg_pt_gp_mem->tg_pt_gp;
		if (!tg_pt_gp) {
			spin_unlock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
			goto check_lu_gp;
		}
		tg_pt_gp_id = tg_pt_gp->tg_pt_gp_id;
		spin_unlock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);

		buf[off] =
			(tpg->se_tpg_tfo->get_fabric_proto_ident(tpg) << 4);
		buf[off++] |= 0x1; /* CODE SET == Binary */
		buf[off] = 0x80; /* Set PIV=1 */
		/* Set ASSOCIATION == target port: 01b */
		buf[off] |= 0x10;
		/* DESIGNATOR TYPE == Target port group identifier */
		buf[off++] |= 0x5;
		off++; /* Skip over Reserved */
		buf[off++] = 4; /* DESIGNATOR LENGTH */
		off += 2; /* Skip over Reserved Field */
		buf[off++] = ((tg_pt_gp_id >> 8) & 0xff);
		buf[off++] = (tg_pt_gp_id & 0xff);
		len += 8; /* Header size + Designation descriptor */
		/*
		 * Logical Unit Group identifier, see spc4r17
		 * section 7.7.3.8
		 */
check_lu_gp:
		lu_gp_mem = dev->dev_alua_lu_gp_mem;
		if (!lu_gp_mem)
			goto check_scsi_name;

		spin_lock(&lu_gp_mem->lu_gp_mem_lock);
		lu_gp = lu_gp_mem->lu_gp;
		if (!lu_gp) {
			spin_unlock(&lu_gp_mem->lu_gp_mem_lock);
			goto check_scsi_name;
		}
		lu_gp_id = lu_gp->lu_gp_id;
		spin_unlock(&lu_gp_mem->lu_gp_mem_lock);

		buf[off++] |= 0x1; /* CODE SET == Binary */
		/* DESIGNATOR TYPE == Logical Unit Group identifier */
		buf[off++] |= 0x6;
		off++; /* Skip over Reserved */
		buf[off++] = 4; /* DESIGNATOR LENGTH */
		off += 2; /* Skip over Reserved Field */
		buf[off++] = ((lu_gp_id >> 8) & 0xff);
		buf[off++] = (lu_gp_id & 0xff);
		len += 8; /* Header size + Designation descriptor */
		/*
		 * SCSI name string designator, see spc4r17
		 * section 7.7.3.11
		 *
		 * Get the PROTOCOL IDENTIFIER as defined by spc4r17
		 * section 7.5.1 Table 362
		 */
check_scsi_name:
		scsi_name_len = strlen(tpg->se_tpg_tfo->tpg_get_wwn(tpg));
		/* UTF-8 ",t,0x<16-bit TPGT>" + NULL Terminator */
		scsi_name_len += 10;
		/* Check for 4-byte padding */
		padding = ((-scsi_name_len) & 3);
		if (padding != 0)
			scsi_name_len += padding;
		/* Header size + Designation descriptor */
		scsi_name_len += 4;

		buf[off] =
			(tpg->se_tpg_tfo->get_fabric_proto_ident(tpg) << 4);
		buf[off++] |= 0x3; /* CODE SET == UTF-8 */
		buf[off] = 0x80; /* Set PIV=1 */
		/* Set ASSOCIATION == target port: 01b */
		buf[off] |= 0x10;
		/* DESIGNATOR TYPE == SCSI name string */
		buf[off++] |= 0x8;
		off += 2; /* Skip over Reserved and length */
		/*
		 * SCSI name string identifer containing, $FABRIC_MOD
		 * dependent information.  For LIO-Target and iSCSI
		 * Target Port, this means "<iSCSI name>,t,0x<TPGT> in
		 * UTF-8 encoding.
		 */
		tpgt = tpg->se_tpg_tfo->tpg_get_tag(tpg);
		scsi_name_len = sprintf(&buf[off], "%s,t,0x%04x",
					tpg->se_tpg_tfo->tpg_get_wwn(tpg), tpgt);
		scsi_name_len += 1 /* Include  NULL terminator */;
		/*
		 * The null-terminated, null-padded (see 4.4.2) SCSI
		 * NAME STRING field contains a UTF-8 format string.
		 * The number of bytes in the SCSI NAME STRING field
		 * (i.e., the value in the DESIGNATOR LENGTH field)
		 * shall be no larger than 256 and shall be a multiple
		 * of four.
		 */
		if (padding)
			scsi_name_len += padding;

		buf[off-1] = scsi_name_len;
		off += scsi_name_len;
		/* Header size + Designation descriptor */
		len += (scsi_name_len + 4);
	}
	buf[2] = ((len >> 8) & 0xff);
	buf[3] = (len & 0xff); /* Page Length for VPD 0x83 */
	return 0;
}

/* Extended INQUIRY Data VPD Page */
static int
target_emulate_evpd_86(struct se_cmd *cmd, unsigned char *buf)
{
	buf[3] = 0x3c;
	/* Set HEADSUP, ORDSUP, SIMPSUP */
	buf[5] = 0x07;

	/* If WriteCache emulation is enabled, set V_SUP */
	if (cmd->se_dev->se_sub_dev->se_dev_attrib.emulate_write_cache > 0)
		buf[6] = 0x01;
	return 0;
}

#if defined(CONFIG_MACH_QNAPTS)
/* Block Limits VPD page */
static int
target_emulate_evpd_b0(struct se_cmd *cmd, unsigned char *buf)
{
	struct se_device *dev = cmd->se_dev;
	int have_tp = 0;
	int opt_sectors_granularity;

	/*
	 * Following sbc3r22 section 6.5.3 Block Limits VPD page, when
	 * emulate_tpu=1 or emulate_tpws=1 we will be expect a
	 * different page length for Thin Provisioning.
	 */
	if (dev->se_sub_dev->se_dev_attrib.emulate_tpu || dev->se_sub_dev->se_dev_attrib.emulate_tpws)
		have_tp = 1;

	buf[0] = dev->transport->get_device_type(dev);



	/*
	 * Set OPTIMAL TRANSFER LENGTH GRANULARITY
	 */
	opt_sectors_granularity = dev->pool_blk_sectors;
	put_unaligned_be16((u16)opt_sectors_granularity, &buf[6]);

	/* Set MAXIMUM TRANSFER LENGTH */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.max_sectors, &buf[8]);

	/*
	 * Set OPTIMAL TRANSFER LENGTH
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.optimal_sectors, &buf[12]);


#if defined(SUPPORT_VAAI)
	/* Here follows the sbc3r31's section 6.5.3 to report the 
	 * page lenghth to 0x3c. */
	buf[3] = 0x3c;
	buf[4] |= u8WriteSameNoZero;
	buf[5] = MAX_ATS_LEN;

	put_unaligned_be64(u64MaxWSLen, &buf[36]);
#else
	buf[3] = have_tp ? 0x3c : 0x10;
	/* Set WSNZ to 1 */
	buf[4] = 0x01;

	/* Exit now if we don't support TP */
	if (!have_tp)
		return 0;
#endif

	/*
	 * Set MAXIMUM UNMAP LBA COUNT
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.max_unmap_lba_count, &buf[20]);

	/*
	 * Set MAXIMUM UNMAP BLOCK DESCRIPTOR COUNT
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.max_unmap_block_desc_count,
			   &buf[24]);

	/*
	 * Set OPTIMAL UNMAP GRANULARITY
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.unmap_granularity, &buf[28]);

	/*
	 * UNMAP GRANULARITY ALIGNMENT
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.unmap_granularity_alignment,
			   &buf[32]);

	if (dev->se_sub_dev->se_dev_attrib.unmap_granularity_alignment != 0)
		buf[32] |= 0x80; /* Set the UGAVALID bit */

	return 0;
}

#else
/* Block Limits VPD page */
static int
target_emulate_evpd_b0(struct se_cmd *cmd, unsigned char *buf)
{
	struct se_device *dev = cmd->se_dev;
	int have_tp = 0;

	/*
	 * Following sbc3r22 section 6.5.3 Block Limits VPD page, when
	 * emulate_tpu=1 or emulate_tpws=1 we will be expect a
	 * different page length for Thin Provisioning.
	 */
	if (dev->se_sub_dev->se_dev_attrib.emulate_tpu || dev->se_sub_dev->se_dev_attrib.emulate_tpws)
		have_tp = 1;

	buf[0] = dev->transport->get_device_type(dev);

	
	buf[3] = have_tp ? 0x3c : 0x10;

	/* Set WSNZ to 1 */
	buf[4] = 0x01;

	/*
	 * Set OPTIMAL TRANSFER LENGTH GRANULARITY
	 */
	put_unaligned_be16(1, &buf[6]);

	/*
	 * Set MAXIMUM TRANSFER LENGTH
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.fabric_max_sectors, &buf[8]);

	/*
	 * Set OPTIMAL TRANSFER LENGTH
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.optimal_sectors, &buf[12]);

	/*
	 * Exit now if we don't support TP.
	 */
	if (!have_tp)
		return 0;

	/*
	 * Set MAXIMUM UNMAP LBA COUNT
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.max_unmap_lba_count, &buf[20]);

	/*
	 * Set MAXIMUM UNMAP BLOCK DESCRIPTOR COUNT
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.max_unmap_block_desc_count,
			   &buf[24]);

	/*
	 * Set OPTIMAL UNMAP GRANULARITY
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.unmap_granularity, &buf[28]);

	/*
	 * UNMAP GRANULARITY ALIGNMENT
	 */
	put_unaligned_be32(dev->se_sub_dev->se_dev_attrib.unmap_granularity_alignment,
			   &buf[32]);

	if (dev->se_sub_dev->se_dev_attrib.unmap_granularity_alignment != 0)
		buf[32] |= 0x80; /* Set the UGAVALID bit */

	return 0;
}

#endif




/* Block Device Characteristics VPD page */
static int
target_emulate_evpd_b1(struct se_cmd *cmd, unsigned char *buf)
{
	struct se_device *dev = cmd->se_dev;

	buf[0] = dev->transport->get_device_type(dev);
	buf[3] = 0x3c;
	buf[5] = dev->se_sub_dev->se_dev_attrib.is_nonrot ? 1 : 0;
	return 0;
}

/* Thin Provisioning VPD */
#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
//adam 20130123
/* 16 bytes NAA plus 4 bytes header, this may be changed in the future */
#define PG_DESC_LEN    (20)
#endif 

static int
target_emulate_evpd_b2(struct se_cmd *cmd, unsigned char *buf)
{
	struct se_device *dev = cmd->se_dev;

	/*
	 * From sbc3r22 section 6.5.4 Thin Provisioning VPD page:
	 *
	 * The PAGE LENGTH field is defined in SPC-4. If the DP bit is set to
	 * zero, then the page length shall be set to 0004h.  If the DP bit
	 * is set to one, then the page length shall be set to the value
	 * defined in table 162.
	 */
	buf[0] = dev->transport->get_device_type(dev);

	/*
	 * Set Hardcoded length mentioned above for DP=0
	 */
	put_unaligned_be16(0x0004, &buf[2]);

	/*
	 * The THRESHOLD EXPONENT field indicates the threshold set size in
	 * LBAs as a power of 2 (i.e., the threshold set size is equal to
	 * 2(threshold exponent)).
	 *
	 * Note that this is currently set to 0x00 as mkp says it will be
	 * changing again.  We can enable this once it has settled in T10
	 * and is actually used by Linux/SCSI ML code.
	 */
	if (dev->se_sub_dev->se_dev_attrib.tp_threshold_enable){
		buf[4] = dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size; // threshold_set_size = 2^buf[4] * 512 bytes = 8k bytes
	}
	else {
		buf[4] = 0x00;
	}

	/*
	 * A TPU bit set to one indicates that the device server supports
	 * the UNMAP command (see 5.25). A TPU bit set to zero indicates
	 * that the device server does not support the UNMAP command.
	 */
	if (dev->se_sub_dev->se_dev_attrib.emulate_tpu != 0)
		buf[5] = 0x80;

	/*
	 * A TPWS bit set to one indicates that the device server supports
	 * the use of the WRITE SAME (16) command (see 5.42) to unmap LBAs.
	 * A TPWS bit set to zero indicates that the device server does not
	 * support the use of the WRITE SAME (16) command to unmap LBAs.
	 */
	if (dev->se_sub_dev->se_dev_attrib.emulate_tpws != 0)
		buf[5] |= 0x40;


#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
//Benjamin 20121105 sync VAAI support from Adam. 
//Benjamin 20121105 FIXME:

	if (dev->se_sub_dev->se_dev_attrib.emulate_tpu
	|| dev->se_sub_dev->se_dev_attrib.emulate_tpws
	)
	{
		/* LBPRZ bit should be the same setting as LBPRZ bit in Read Capacity 16 */
		if(bReturnZeroWhenReadUnmapLba)
			buf[5] |= 0x04;

		if(bSupportArchorLba)
			buf[5] |= 0x02;

		buf[6] |= VPD_B2h_CURRENT_PROVISION_TYPE;
		if((buf[6] & 0x07) != VPD_B2h_PROVISION_TYPE_NONE){
			/*
			 * FIXED ME
			 *
			 * Here to report the PROVISIONING GROUP DESCRIPTOR
			 * information. The PROVISIONING GROUP DESCRIPTOR field
			 * contains a designation descriptor for the LBA
			 * mapping resources used by logical unit.
			 *
			 * The ASSOCIATION field should be set to 00b
			 * The DESIGNATOR TYPE field should be 01h
			 * (T10 vendor ID based) or 03h (NAA)
			 *
			 * NOTE: 
			 * This code depends on target_emulate_evpd_83(), 
			 * please take care it...
			 */

			// Benjamin 20130123 for BUG 30250

			/* Set Hardcoded length mentioned above for DP=1 and
			 * set the DP bit in byte[5] to 1 anyway.
			 */
			put_unaligned_be16(4 + PG_DESC_LEN, &buf[2]);
			buf[5] |= 0x1;
			  
			/* SBC3R31, page 279 
			 *
			 * FIXED ME !!  we need to understand what the purpose
			 * of provisioning group descriptor is.
			 */			
			vaai_build_pg_desc(cmd, buf);
		}
	}
#endif  /* #if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP) */

	return 0;
}

static int
target_emulate_evpd_00(struct se_cmd *cmd, unsigned char *buf);

static struct {
	uint8_t		page;
	int		(*emulate)(struct se_cmd *, unsigned char *);
} evpd_handlers[] = {
	{ .page = 0x00, .emulate = target_emulate_evpd_00 },
	{ .page = 0x80, .emulate = target_emulate_evpd_80 },
	{ .page = 0x83, .emulate = target_emulate_evpd_83 },
	{ .page = 0x86, .emulate = target_emulate_evpd_86 },

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TPC_CMD)
	{ .page = 0x8f, .emulate = tpc_emulate_evpd_8f    },
#endif
#endif

	{ .page = 0xb0, .emulate = target_emulate_evpd_b0 },
	{ .page = 0xb1, .emulate = target_emulate_evpd_b1 },
	{ .page = 0xb2, .emulate = target_emulate_evpd_b2 },
};

/* supported vital product data pages */
static int
target_emulate_evpd_00(struct se_cmd *cmd, unsigned char *buf)
{
	int p;

	/*
	 * Only report the INQUIRY EVPD=1 pages after a valid NAA
	 * Registered Extended LUN WWN has been set via ConfigFS
	 * during device creation/restart.
	 */
	if (cmd->se_dev->se_sub_dev->su_dev_flags &
			SDF_EMULATED_VPD_UNIT_SERIAL) {
		buf[3] = ARRAY_SIZE(evpd_handlers);
		for (p = 0; p < ARRAY_SIZE(evpd_handlers); ++p)
			buf[p + 4] = evpd_handlers[p].page;
	}

	return 0;
}

int target_emulate_inquiry(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	struct se_portal_group *tpg = cmd->se_lun->lun_sep->sep_tpg;
	unsigned char *buf, *map_buf;
	unsigned char *cdb = cmd->t_task_cdb;
	int p, ret;

	map_buf = transport_kmap_data_sg(cmd);
	/*
	 * If SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC is not set, then we
	 * know we actually allocated a full page.  Otherwise, if the
	 * data buffer is too small, allocate a temporary buffer so we
	 * don't have to worry about overruns in all our INQUIRY
	 * emulation handling.
	 */
#ifdef CONFIG_MACH_QNAPTS   //Benjamin 20121102: map_buf may be NULL, and need to be reset.
	if (cmd->data_length < SE_INQUIRY_BUF &&
	    (cmd->se_cmd_flags & SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC)) {
		buf = kzalloc(SE_INQUIRY_BUF, GFP_KERNEL);
		if (!buf) {
			transport_kunmap_data_sg(cmd);
			cmd->scsi_sense_reason = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			return -ENOMEM;
		}
	} else {
		buf = map_buf;
	}
#else
	buf = (cmd->data_length < SE_INQUIRY_BUF &&
			(cmd->se_cmd_flags & SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC)) ? 
			kzalloc(SE_INQUIRY_BUF, GFP_KERNEL) : map_buf;

	if (!buf) {
		transport_kunmap_data_sg(cmd);
		cmd->scsi_sense_reason = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		return -ENOMEM;
	}    
	// FIXME: Patch Re-add explict zeroing of INQUIRY bounce buffer memory 20121101 or not ? Not the same way...
#endif  /* #ifdef CONFIG_MACH_QNAPTS */

	if (dev == tpg->tpg_virt_lun0.lun_se_dev)
		buf[0] = 0x3f; /* Not connected */
	else
		buf[0] = dev->transport->get_device_type(dev);

	if (!(cdb[1] & 0x1)) {
		if (cdb[2]) {
			pr_err("INQUIRY with EVPD==0 but PAGE CODE=%02x\n",
			       cdb[2]);
			cmd->scsi_sense_reason = TCM_INVALID_CDB_FIELD;
			ret = -EINVAL;
			goto out;
		}

		ret = target_emulate_inquiry_std(cmd, buf);
		goto out;
	}

	for (p = 0; p < ARRAY_SIZE(evpd_handlers); ++p) {
		if (cdb[2] == evpd_handlers[p].page) {
			buf[1] = cdb[2];
			ret = evpd_handlers[p].emulate(cmd, buf);
			goto out;
		}
	}

	pr_err("Unknown VPD Code: 0x%02x\n", cdb[2]);
	cmd->scsi_sense_reason = TCM_INVALID_CDB_FIELD;
	ret = -EINVAL;

out:
	if (buf != map_buf) {
		memcpy(map_buf, buf, cmd->data_length);
		kfree(buf);
	}
	transport_kunmap_data_sg(cmd);

	if (!ret) {
		task->task_scsi_status = GOOD;
		transport_complete_task(task, 1);
	}
	return ret;
}

int target_emulate_readcapacity(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	unsigned char *buf;
	unsigned long long blocks_long = dev->transport->get_blocks(dev);
	u32 blocks;

	if (blocks_long >= 0x00000000ffffffff)
		blocks = 0xffffffff;
	else
		blocks = (u32)blocks_long;

	buf = transport_kmap_data_sg(cmd);

	buf[0] = (blocks >> 24) & 0xff;
	buf[1] = (blocks >> 16) & 0xff;
	buf[2] = (blocks >> 8) & 0xff;
	buf[3] = blocks & 0xff;
	buf[4] = (dev->se_sub_dev->se_dev_attrib.block_size >> 24) & 0xff;
	buf[5] = (dev->se_sub_dev->se_dev_attrib.block_size >> 16) & 0xff;
	buf[6] = (dev->se_sub_dev->se_dev_attrib.block_size >> 8) & 0xff;
	buf[7] = dev->se_sub_dev->se_dev_attrib.block_size & 0xff;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP) 
    	/* for run-time capacity change check */
	if ( dev->se_sub_dev->se_dev_attrib.lun_blocks != blocks ){
		dev->se_sub_dev->se_dev_attrib.lun_blocks = blocks;
	}

#endif

	transport_kunmap_data_sg(cmd);

	task->task_scsi_status = GOOD;
	transport_complete_task(task, 1);
	return 0;
}

int target_emulate_readcapacity_16(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	unsigned char *buf;
	unsigned long long blocks = dev->transport->get_blocks(dev);

//#if defined(SUPPORT_FAST_BLOCK_CLONE)
#if 0
// Jay Wei,20150303, QKS task #11911, correct the issue that physical block size is 512K
	int bs_order, value;
#endif

	buf = transport_kmap_data_sg(cmd);

	buf[0] = (blocks >> 56) & 0xff;
	buf[1] = (blocks >> 48) & 0xff;
	buf[2] = (blocks >> 40) & 0xff;
	buf[3] = (blocks >> 32) & 0xff;
	buf[4] = (blocks >> 24) & 0xff;
	buf[5] = (blocks >> 16) & 0xff;
	buf[6] = (blocks >> 8) & 0xff;
	buf[7] = blocks & 0xff;
	buf[8] = (dev->se_sub_dev->se_dev_attrib.block_size >> 24) & 0xff;
	buf[9] = (dev->se_sub_dev->se_dev_attrib.block_size >> 16) & 0xff;
	buf[10] = (dev->se_sub_dev->se_dev_attrib.block_size >> 8) & 0xff;
	buf[11] = dev->se_sub_dev->se_dev_attrib.block_size & 0xff;

	/*
	 * Set Thin Provisioning Enable bit following sbc3r22 in section
	 * READ CAPACITY (16) byte 14 if emulate_tpu or emulate_tpws is enabled.
	 */
	if (dev->se_sub_dev->se_dev_attrib.emulate_tpu || dev->se_sub_dev->se_dev_attrib.emulate_tpws)
		buf[14] = 0x80;

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TP)
//Benjamin 20121105 sync VAAI support from Adam. 
	if (dev->se_sub_dev->se_dev_attrib.emulate_tpu
	|| dev->se_sub_dev->se_dev_attrib.emulate_tpws)
	{
		if(bReturnZeroWhenReadUnmapLba)
			buf[14] |= 0x40;
	}

	/* for run-time capacity change check */
	if ( dev->se_sub_dev->se_dev_attrib.lun_blocks != blocks ){
		dev->se_sub_dev->se_dev_attrib.lun_blocks = blocks;
	}
#endif

//#if defined(SUPPORT_FAST_BLOCK_CLONE)
#if 0
// Jay Wei,20150303, QKS task #11911, correct the issue that physical block size is 512K
	bs_order = ilog2(dev->se_sub_dev->se_dev_attrib.block_size);
	value = ilog2(((POOL_BLK_SIZE_KB << 10) >> bs_order));

	if (value & (~(0x0f)))
		pr_err("error !!, order value range is from bit0 to bit3\n");

	/* bit[0-3] for LOGICAL BLOCKS PER PHYSICAL BLOCK EXPONENT */
	buf[13] = value;

	/* bit[0-5] for byte14 and bit[0-7] for byte15 is 
	 * LOWEST ALIGNED LOGICAL BLOCK ADDRESS
	 */
	value = (ALIGN_GAP_SIZE_B >> bs_order);
	buf[14] |= ((value >> 8) & 0x3f);
	buf[15] = (value & 0xff);
#endif
#endif

	transport_kunmap_data_sg(cmd);

	task->task_scsi_status = GOOD;
	transport_complete_task(task, 1);
	return 0;
}

static int
target_modesense_rwrecovery(unsigned char *p)
{
	p[0] = 0x01;
	p[1] = 0x0a;

	return 12;
}

static int
target_modesense_rwrecovery_tp_threshold(unsigned char *p)
{
	p[0] = 0x01;
	p[1] = 0x0a;

	p[7] |= 0x80; // LBPERE set to one specifes that thin-provision threshold notification is enabled
	
	return 12;	
}

#ifdef CONFIG_MACH_QNAPTS  // Benjamin 20110812 for read-only snapshot
static int
target_modesense_control(struct se_device *dev, int mode, unsigned char *p)
#else
static int
target_modesense_control(struct se_device *dev, unsigned char *p)
#endif 
{
	p[0] = 0x0a;
	p[1] = 0x0a;
	p[2] = 2;
	/*
	 * From spc4r23, 7.4.7 Control mode page
	 *
	 * The QUEUE ALGORITHM MODIFIER field (see table 368) specifies
	 * restrictions on the algorithm used for reordering commands
	 * having the SIMPLE task attribute (see SAM-4).
	 *
	 *                    Table 368 -- QUEUE ALGORITHM MODIFIER field
	 *                         Code      Description
	 *                          0h       Restricted reordering
	 *                          1h       Unrestricted reordering allowed
	 *                          2h to 7h    Reserved
	 *                          8h to Fh    Vendor specific
	 *
	 * A value of zero in the QUEUE ALGORITHM MODIFIER field specifies that
	 * the device server shall order the processing sequence of commands
	 * having the SIMPLE task attribute such that data integrity is maintained
	 * for that I_T nexus (i.e., if the transmission of new SCSI transport protocol
	 * requests is halted at any time, the final value of all data observable
	 * on the medium shall be the same as if all the commands had been processed
	 * with the ORDERED task attribute).
	 *
	 * A value of one in the QUEUE ALGORITHM MODIFIER field specifies that the
	 * device server may reorder the processing sequence of commands having the
	 * SIMPLE task attribute in any manner. Any data integrity exposures related to
	 * command sequence order shall be explicitly handled by the application client
	 * through the selection of appropriate ommands and task attributes.
	 */
	p[3] = (dev->se_sub_dev->se_dev_attrib.emulate_rest_reord == 1) ? 0x00 : 0x10;
	/*
	 * From spc4r17, section 7.4.6 Control mode Page
	 *
	 * Unit Attention interlocks control (UN_INTLCK_CTRL) to code 00b
	 *
	 * 00b: The logical unit shall clear any unit attention condition
	 * reported in the same I_T_L_Q nexus transaction as a CHECK CONDITION
	 * status and shall not establish a unit attention condition when a com-
	 * mand is completed with BUSY, TASK SET FULL, or RESERVATION CONFLICT
	 * status.
	 *
	 * 10b: The logical unit shall not clear any unit attention condition
	 * reported in the same I_T_L_Q nexus transaction as a CHECK CONDITION
	 * status and shall not establish a unit attention condition when
	 * a command is completed with BUSY, TASK SET FULL, or RESERVATION
	 * CONFLICT status.
	 *
	 * 11b a The logical unit shall not clear any unit attention condition
	 * reported in the same I_T_L_Q nexus transaction as a CHECK CONDITION
	 * status and shall establish a unit attention condition for the
	 * initiator port associated with the I_T nexus on which the BUSY,
	 * TASK SET FULL, or RESERVATION CONFLICT status is being returned.
	 * Depending on the status, the additional sense code shall be set to
	 * PREVIOUS BUSY STATUS, PREVIOUS TASK SET FULL STATUS, or PREVIOUS
	 * RESERVATION CONFLICT STATUS. Until it is cleared by a REQUEST SENSE
	 * command, a unit attention condition shall be established only once
	 * for a BUSY, TASK SET FULL, or RESERVATION CONFLICT status regardless
	 * to the number of commands completed with one of those status codes.
	 */
	p[4] = (dev->se_sub_dev->se_dev_attrib.emulate_ua_intlck_ctrl == 2) ? 0x30 :
	       (dev->se_sub_dev->se_dev_attrib.emulate_ua_intlck_ctrl == 1) ? 0x20 : 0x00;
#ifdef CONFIG_MACH_QNAPTS  // Benjamin 20110812 for read-only snapshot
    // set SWP for snapshot
    p[4] |= (mode) ? 0x08 : 0;
#endif                      
	/*
	 * From spc4r17, section 7.4.6 Control mode Page
	 *
	 * Task Aborted Status (TAS) bit set to zero.
	 *
	 * A task aborted status (TAS) bit set to zero specifies that aborted
	 * tasks shall be terminated by the device server without any response
	 * to the application client. A TAS bit set to one specifies that tasks
	 * aborted by the actions of an I_T nexus other than the I_T nexus on
	 * which the command was received shall be completed with TASK ABORTED
	 * status (see SAM-4).
	 */
	p[5] = (dev->se_sub_dev->se_dev_attrib.emulate_tas) ? 0x40 : 0x00;
	p[8] = 0xff;
	p[9] = 0xff;
	p[11] = 30;

	return 12;
}

static int
target_modesense_caching(struct se_device *dev, unsigned char *p)
{
	p[0] = 0x08;
	p[1] = 0x12;
	if (dev->se_sub_dev->se_dev_attrib.emulate_write_cache > 0)
		p[2] = 0x04; /* Write Cache Enable */
	p[12] = 0x20; /* Disabled Read Ahead */

	return 20;
}

static void
target_modesense_write_protect(unsigned char *buf, int type)
{
	/*
	 * I believe that the WP bit (bit 7) in the mode header is the same for
	 * all device types..
	 */
	switch (type) {
	case TYPE_DISK:
	case TYPE_TAPE:
	default:
		buf[0] |= 0x80; /* WP bit */
		break;
	}
}

static void
target_modesense_dpofua(unsigned char *buf, int type)
{
	switch (type) {
	case TYPE_DISK:
		buf[0] |= 0x10; /* DPOFUA bit */
		break;
	default:
		break;
	}
}

int target_emulate_modesense(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	
	/* Jay Wei, 20140901, Make the type of *cdb unsigned char
	    so that its type will be consistent with cmd->t_task_cdb. */
	unsigned char *cdb = cmd->t_task_cdb;
	unsigned char *rbuf;
	int type = dev->transport->get_device_type(dev);
	int ten = (cmd->t_task_cdb[0] == MODE_SENSE_10);
	int offset = ten ? 8 : 4;
	int length = 0;
	unsigned char buf[SE_MODE_PAGE_BUF];
#ifdef CONFIG_MACH_QNAPTS
	u32 syswp;
	unsigned long flags;

	// Benjamin 20110812 for read-only snapshot
	int mode = ((cmd->se_lun->lun_access & TRANSPORT_LUNFLAGS_READ_ONLY) ||
		(cmd->se_deve &&
		(cmd->se_deve->lun_flags & TRANSPORT_LUNFLAGS_READ_ONLY))) ? 
		1 : 0;


	spin_lock_irqsave(&dev->se_sub_dev->se_dev_lock, flags);
	syswp = dev->se_sub_dev->se_dev_attrib.syswp;
	spin_unlock_irqrestore(&dev->se_sub_dev->se_dev_lock, flags);

	mode |= ((syswp == 1) ? 1 : 0);
#endif

	memset(buf, 0, SE_MODE_PAGE_BUF);

	switch (cdb[2] & 0x3f) {

 /* Jay Wei, 20140901, Improve mode sense. Redmine #4947 */
	case 0x01:
#ifdef CONFIG_MACH_QNAPTS

        /* Jay Wei, 20140901, When subpage is FFh, by rule target needs to return
                * all implemented subpages of page code 01h.              */                
		if(cdb[3] == 0x00 || cdb[3] == 0xff) //subpage 00h, FFh
		{   /* Turn to use "se_dev_provision" to determine. */
            if(!strcmp(dev->se_sub_dev->se_dev_provision, "thin")){ 
   				length = target_modesense_rwrecovery_tp_threshold(&buf[offset]);
    			dev->se_sub_dev->se_dev_attrib.tp_threshold_hit = 0;				
			}
			else{
                length = target_modesense_rwrecovery(&buf[offset]);
			}
			break;
		}
        else
            /* Use "goto" to let unimplemented pages go to the default of switch-case,
                          or they would go through and cause wrong results. */                       
            goto NOT_SUPPORTED;        
#else
		length = target_modesense_rwrecovery(&buf[offset]);
		break;
#endif

	case 0x08:
#ifdef CONFIG_MACH_QNAPTS	

        /* Jay Wei, 20140901, Same story with page 01 */
		if(cdb[3] == 0x00 || cdb[3] == 0xff) //subpage 00h, FFh
		{
			length = target_modesense_caching(dev, &buf[offset]);
			break;
		}
        else
            goto NOT_SUPPORTED;
#else
		length = target_modesense_caching(dev, &buf[offset]);
		break;
#endif

	case 0x0a:
#ifdef CONFIG_MACH_QNAPTS  // Benjamin 20110812 for read-only snapshot

        /* Jay Wei, 20140901, Same story with page 01*/
		if(cdb[3] == 0x00 || cdb[3] == 0xff) //subpage 00h, FFh
		{
			length = target_modesense_control(dev, mode, &buf[offset]);
			break;
		}
        else
            goto NOT_SUPPORTED;        

#else        
		length = target_modesense_control(dev, &buf[offset]);
		break;
#endif

	case 0x3f:
#ifdef CONFIG_MACH_QNAPTS		
		if(cdb[3] == 0x00) //subpage 00h. Return all implemented pages with subpage code 00h.
		{
            /* 01h,00h */
            if(!strcmp(dev->se_sub_dev->se_dev_provision, "thin")){
   				length = target_modesense_rwrecovery_tp_threshold(&buf[offset]);
			}
			else{
                length = target_modesense_rwrecovery(&buf[offset]);
			}
            
            /* 08h,00h */
			length += target_modesense_caching(dev, &buf[offset+length]);
            
			// 0Ah,00h  Benjamin 20110812 for read-only snapshot.   
			length += target_modesense_control(dev, mode, &buf[offset+length]);
			break;
		}
        
        /* Jay Wei, 20140901, Return all implemented mode pages with all subpages when subpage is FFh */
        else if(cdb[3] == 0xff) //subpage FFh
        {        
            /* 01h,00h */
            if(!strcmp(dev->se_sub_dev->se_dev_provision, "thin")){
   				length = target_modesense_rwrecovery_tp_threshold(&buf[offset]);
    			dev->se_sub_dev->se_dev_attrib.tp_threshold_hit = 0; 				
			}
			else{
                length = target_modesense_rwrecovery(&buf[offset]);
			}
            /* 08h,00h */
            length += target_modesense_caching(dev, &buf[offset+length]);
                
            // Benjamin 20110812 for read-only snapshot.   0Ah,00h
            length += target_modesense_control(dev, mode, &buf[offset+length]);
            
#if defined(SUPPORT_TP) 
            /* 1Ch,02h */
            if (dev->se_sub_dev->se_dev_attrib.emulate_tpu
			|| dev->se_sub_dev->se_dev_attrib.emulate_tpws)
            {
        		length += vaai_modesense_lbp(cmd, &buf[offset+length]);
				break;
	        }         
            else
                break; // Won't print anything because this is for all pages.      
#endif
    
        }
        else
            goto NOT_SUPPORTED;        

#endif

/* Jay Wei 20140901, NOTE! Here uses   #ifndef   to keep the primitive LIO code. */    
#ifndef CONFIG_MACH_QNAPTS       

        length = target_modesense_rwrecovery(&buf[offset]);
        length += target_modesense_caching(dev, &buf[offset+length]);
        length += target_modesense_control(dev, &buf[offset+length]);
		break;        
#endif

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
//Benjamin 20121105 sync VAAI support from Adam. 
	case 0x1c:
		/* Jay Wei, 20140901, add subpage 0xff temporary because the behavior
		  * is equivalent. */
		if(cdb[3] == 0x02 || cdb[3] == 0xff){
			if (dev->se_sub_dev->se_dev_attrib.emulate_tpu
			|| dev->se_sub_dev->se_dev_attrib.emulate_tpws
			)
			{
				length = vaai_modesense_lbp(cmd, &buf[offset]);
				break;
			}else{
				pr_err("MODE SENSE: page/subpage: "
					"0x%02x/0x%02x not supported "
					"on thick device\n", cdb[2] & 0x3f, 
					cdb[3]);

				__set_err_reason(ERR_INVALID_CDB_FIELD, 
					&cmd->scsi_sense_reason);
				return -EOPNOTSUPP;
			}
		}
        else
            goto NOT_SUPPORTED;        
#endif

NOT_SUPPORTED:

	default:
		pr_err("MODE SENSE: unimplemented page/subpage: 0x%02x/0x%02x\n",
		       cdb[2] & 0x3f, cdb[3]);
		cmd->scsi_sense_reason = TCM_UNKNOWN_MODE_PAGE;
		return -EINVAL;
	}
	offset += length;

	if (ten) {
		offset -= 2;
		buf[0] = (offset >> 8) & 0xff;
		buf[1] = offset & 0xff;

		if ((cmd->se_lun->lun_access & TRANSPORT_LUNFLAGS_READ_ONLY) ||
		    (cmd->se_deve &&
		    (cmd->se_deve->lun_flags & TRANSPORT_LUNFLAGS_READ_ONLY)))
			target_modesense_write_protect(&buf[3], type);


		if (syswp)
			target_modesense_write_protect(&buf[3], type);

		if ((dev->se_sub_dev->se_dev_attrib.emulate_write_cache > 0) &&
		    (dev->se_sub_dev->se_dev_attrib.emulate_fua_write > 0))
			target_modesense_dpofua(&buf[3], type);

		if ((offset + 2) > cmd->data_length)
			offset = cmd->data_length;
	} else {
		offset -= 1;
		buf[0] = offset & 0xff;

		if ((cmd->se_lun->lun_access & TRANSPORT_LUNFLAGS_READ_ONLY) ||
		    (cmd->se_deve &&
		    (cmd->se_deve->lun_flags & TRANSPORT_LUNFLAGS_READ_ONLY)))
			target_modesense_write_protect(&buf[2], type);


		if (syswp)
			target_modesense_write_protect(&buf[2], type);

		if ((dev->se_sub_dev->se_dev_attrib.emulate_write_cache > 0) &&
		    (dev->se_sub_dev->se_dev_attrib.emulate_fua_write > 0))
			target_modesense_dpofua(&buf[2], type);

		if ((offset + 1) > cmd->data_length)
			offset = cmd->data_length;
	}

#ifdef CONFIG_MACH_QNAPTS 
	//  Benjamin 20130115: Make the Page Length be consistent with DataSegmentLength.
	cmd->data_length = ten ? offset + 2 : offset + 1;

	/* spc4r36, p601
	 * Fix code about length doesn't plus 2 bytes for mode data length
	 * before to copy data
	 */
	offset = cmd->data_length;
#endif


	rbuf = transport_kmap_data_sg(cmd);
	memcpy(rbuf, buf, offset);
	transport_kunmap_data_sg(cmd);

	task->task_scsi_status = GOOD;
	transport_complete_task(task, 1);
	return 0;
}

int target_emulate_request_sense(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	unsigned char *cdb = cmd->t_task_cdb;
	unsigned char *buf;
	u8 ua_asc = 0, ua_ascq = 0;
	int err = 0;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
	struct se_device *dev = cmd->se_dev;
	unsigned long long blocks = dev->transport->get_blocks(dev);
#endif	
	
	if (cdb[1] & 0x01) {
		pr_err("REQUEST_SENSE description emulation not"
			" supported\n");
		cmd->scsi_sense_reason = TCM_INVALID_CDB_FIELD;
		return -ENOSYS;
	}

	buf = transport_kmap_data_sg(cmd);

	if (!core_scsi3_ua_clear_for_request_sense(cmd, &ua_asc, &ua_ascq)) {
		/*
		 * CURRENT ERROR, UNIT ATTENTION
		 */
		buf[0] = 0x70;
		buf[SPC_SENSE_KEY_OFFSET] = UNIT_ATTENTION;

		if (cmd->data_length < 18) {
			buf[7] = 0x00;
			err = -EINVAL;
			goto end;
		}
		/*
		 * The Additional Sense Code (ASC) from the UNIT ATTENTION
		 */
		buf[SPC_ASC_KEY_OFFSET] = ua_asc;
		buf[SPC_ASCQ_KEY_OFFSET] = ua_ascq;
		buf[7] = 0x0A;
	} else {
		/*
		 * CURRENT ERROR, NO SENSE
		 */
		buf[0] = 0x70;
		buf[SPC_SENSE_KEY_OFFSET] = NO_SENSE;

		if (cmd->data_length < 18) {
			buf[7] = 0x00;
			err = -EINVAL;
			goto end;
		}
		/*
		 * NO ADDITIONAL SENSE INFORMATION
		 */
		buf[SPC_ASC_KEY_OFFSET] = 0x00;
		buf[7] = 0x0A;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
    		/* for run-time capacity change check */
		if ( dev->se_sub_dev->se_dev_attrib.lun_blocks != blocks ){
			cmd->transport_state = CMD_T_CAP_CHANGE;	
		}
		
#endif
		
	}

end:
	transport_kunmap_data_sg(cmd);
	task->task_scsi_status = GOOD;
	transport_complete_task(task, 1);
	return 0;
}

/*
 * Used for TCM/IBLOCK and TCM/FILEIO for block/blk-lib.c level discard support.
 * Note this is not used for TCM/pSCSI passthrough
 */
#if defined(CONFIG_MACH_QNAPTS)
int __target_pre_cond_emulate_unmap(
	LIO_SE_CMD *se_cmd,
	unsigned char *buf,
	ERR_REASON_INDEX *err_idx,
	int *need_do_cmd
	)
{
	/* buf is parameter data and was mapped by transport_kunmap_data_sg already */

	LIO_SE_DEVICE *se_dev = se_cmd->se_dev;
	int size = se_cmd->data_length;
	int dl, bd_dl;

	/* if param list len is zero, means no data shall be transferred */
	if (!size){
		*need_do_cmd = 0;
		return 0;
	}

	if (!is_thin_lun(se_dev)){
		pr_err("[%s]: UNMAP emulation can't work on thick LU\n", __func__);
		*err_idx = ERR_UNKNOWN_SAM_OPCODE;
		return -EINVAL;
	}
	
	/* sbc3r35, page 186 */
	if ((se_cmd->t_task_cdb[1] & 0x1) && (bSupportArchorLba == 0)){
		pr_err("[%s]: Not support ANCHOR in UNMAP emulation\n", __func__);
		*err_idx = ERR_INVALID_CDB_FIELD;
		return -EOPNOTSUPP;
	}

	/* Spec said the param list len shall be large than 8 bytes */
	if (size < 8){
		pr_err("[%s]: Param list len shall be large than 8 bytes in "
				"UNMAP emulation\n", __func__);
		*err_idx = ERR_PARAMETER_LIST_LEN_ERROR;
		return -EINVAL;
	}
	
	if (!se_dev->transport->do_discard) {
		pr_err("[%s]: UNMAP emulation not supported on %s\n", __func__,
			se_dev->transport->name);
		*err_idx = ERR_UNKNOWN_SAM_OPCODE;
		return -ENOSYS;
	}


	dl = get_unaligned_be16(&buf[0]);
	bd_dl = get_unaligned_be16(&buf[2]);

	/* sbc3r35, page 188 */
	if (dl != (bd_dl + 6)){
		pr_err("[%s]: UNMAP data len doesn't equal to UNMAP blk "
			"desc data len plus 6 bytes in UNMAP emulation\n", __func__);
		*err_idx = ERR_PARAMETER_LIST_LEN_ERROR;
		return -EINVAL;
	}

	/* if UNMAP blk desc data len is zero, means no any unmap operation */
	if (!bd_dl){
		*need_do_cmd = 0;
		return 0;
	}

	/* sbc3r35, p 164 */
	size = min(size - 8, bd_dl);
	
	if (size < 16){
		*err_idx = ERR_INVALID_PARAMETER_LIST;
		return -EINVAL;
	}

	if (size / 16 > se_dev->se_sub_dev->se_dev_attrib.max_unmap_block_desc_count) {
		*err_idx = ERR_INVALID_PARAMETER_LIST;
		return -EINVAL;
	}

	return 0;

}

int target_emulate_unmap(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	unsigned char *buf = NULL, *ptr = NULL;
	sector_t lba;
	int size = cmd->data_length;
	int dl, bd_dl, need_do_cmd = 1, ret = 0;
	u32 range;
	ERR_REASON_INDEX err_idx = MAX_ERR_REASON_INDEX;

	buf = transport_kmap_data_sg(cmd);
	if (!buf){
		__set_err_reason(ERR_PARAMETER_LIST_LEN_ERROR, &cmd->scsi_sense_reason);
		return -ENOMEM;
	}

	ret = __target_pre_cond_emulate_unmap(cmd, buf, 
			&err_idx, &need_do_cmd);

	if (ret != 0){
		__set_err_reason(err_idx, &cmd->scsi_sense_reason);
		goto _EXIT_;
	}

	/* please see __target_pre_cond_emulate_unmap */
	if (need_do_cmd == 0)
		goto _EXIT_;

	dl = get_unaligned_be16(&buf[0]);
	bd_dl = get_unaligned_be16(&buf[2]);
	size = min(size - 8, bd_dl);

	/* First UNMAP block descriptor starts at 8 byte offset */
	ptr = &buf[8];
	pr_debug("UNMAP: Sub: %s Using dl: %u bd_dl: %u size: %u"
		" ptr: %p\n", dev->transport->name, dl, bd_dl, size, ptr);

	while (size >= 16) {
		lba = get_unaligned_be64(&ptr[0]);
		range = get_unaligned_be32(&ptr[8]);

		pr_debug("UNMAP: Using lba: %llu and range: %u\n",
				 (unsigned long long)lba, range);

		/* sbc3r35, p 164 */
		if (range > dev->se_sub_dev->se_dev_attrib.max_unmap_lba_count) {
			cmd->scsi_sense_reason = TCM_INVALID_PARAMETER_LIST;
			ret = -EINVAL;
			goto _EXIT_;
		}

		/* sbc3r35, p 164 */
		if (lba + range > dev->transport->get_blocks(dev) + 1) {
			cmd->scsi_sense_reason = TCM_ADDRESS_OUT_OF_RANGE;
			ret = -EINVAL;
			goto _EXIT_;
		}

		/* Note:
		 * We will truncate the page cache under file i/o configuration
		 * in unmap code. Therefore, we won't convert lba, range
		 * under 4kb logical block size environment here. 
		 * We will do it in backend code
		 */

		/* 20140626, adamhsu, redmine 8745,8777,8778 */
		ret = dev->transport->do_discard(cmd, lba, range);

		if (ret < 0) {
			pr_err("transport->do_discard() failed: %d\n", ret);
			goto _EXIT_;
		}

		if ((qnap_transport_is_dropped_by_release_conn(cmd) == true)
		|| (qnap_transport_is_dropped_by_tmr(cmd) == true)
		)
			goto _EXIT_;

		ptr += 16;
		size -= 16;
	}

_EXIT_:
	if (buf)
		transport_kunmap_data_sg(cmd);

	if (!ret) {
		task->task_scsi_status = GOOD;
		transport_complete_task(task, 1);
	}
	return ret;
}

#else
int target_emulate_unmap(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	unsigned char *buf, *ptr = NULL;
	sector_t lba;
	int size = cmd->data_length;
	u32 range;
	int ret = 0;
	int dl, bd_dl;

	if (!dev->transport->do_discard) {
		pr_err("UNMAP emulation not supported on %s\n",
				dev->transport->name);
		cmd->scsi_sense_reason = TCM_UNSUPPORTED_SCSI_OPCODE;
		return -ENOSYS;
	}

	buf = transport_kmap_data_sg(cmd);

	dl = get_unaligned_be16(&buf[0]);
	bd_dl = get_unaligned_be16(&buf[2]);

	size = min(size - 8, bd_dl);

	if (size / 16 > dev->se_sub_dev->se_dev_attrib.max_unmap_block_desc_count) {
		cmd->scsi_sense_reason = TCM_INVALID_PARAMETER_LIST;
		ret = -EINVAL;
		goto err;
	}

	/* First UNMAP block descriptor starts at 8 byte offset */
	ptr = &buf[8];
	pr_debug("UNMAP: Sub: %s Using dl: %u bd_dl: %u size: %u"
		" ptr: %p\n", dev->transport->name, dl, bd_dl, size, ptr);

	while (size >= 16) {
		lba = get_unaligned_be64(&ptr[0]);
		range = get_unaligned_be32(&ptr[8]);

		pr_debug("UNMAP: Using lba: %llu and range: %u\n",
				 (unsigned long long)lba, range);

		if (range > dev->se_sub_dev->se_dev_attrib.max_unmap_lba_count) {
			cmd->scsi_sense_reason = TCM_INVALID_PARAMETER_LIST;
			ret = -EINVAL;
			goto err;
		}

		if (lba + range > dev->transport->get_blocks(dev) + 1) {
			cmd->scsi_sense_reason = TCM_ADDRESS_OUT_OF_RANGE;
			ret = -EINVAL;
			goto err;
		}

		ret = dev->transport->do_discard(dev, lba, range);

		if (ret < 0) {
			pr_err("transport->do_discard() failed: %d\n", ret);
			goto err;
		}

		ptr += 16;
		size -= 16;
	}

err:
	transport_kunmap_data_sg(cmd);
	if (!ret) {
		task->task_scsi_status = GOOD;
		transport_complete_task(task, 1);
	}
	return ret;
}
#endif

/*
 * Used for TCM/IBLOCK and TCM/FILEIO for block/blk-lib.c level discard support.
 * Note this is not used for TCM/pSCSI passthrough
 */
int target_emulate_write_same(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	sector_t range;
	sector_t lba = cmd->t_task_lba;
	u32 num_blocks;
	int ret;

	if (!dev->transport->do_discard) {
		pr_err("WRITE_SAME emulation not supported"
				" for: %s\n", dev->transport->name);
		cmd->scsi_sense_reason = TCM_UNSUPPORTED_SCSI_OPCODE;
		return -ENOSYS;
	}

	if (cmd->t_task_cdb[0] == WRITE_SAME)
		num_blocks = get_unaligned_be16(&cmd->t_task_cdb[7]);
	else if (cmd->t_task_cdb[0] == WRITE_SAME_16)
		num_blocks = get_unaligned_be32(&cmd->t_task_cdb[10]);
	else /* WRITE_SAME_32 via VARIABLE_LENGTH_CMD */
		num_blocks = get_unaligned_be32(&cmd->t_task_cdb[28]);

	/*
	 * Use the explicit range when non zero is supplied, otherwise calculate
	 * the remaining range based on ->get_blocks() - starting LBA.
	 */
	if (num_blocks != 0)
		range = num_blocks;
	else
		range = (dev->transport->get_blocks(dev) - lba) + 1;

	pr_debug("WRITE_SAME UNMAP: LBA: %llu Range: %llu\n",
		 (unsigned long long)lba, (unsigned long long)range);

	ret = dev->transport->do_discard(dev, lba, range);
	if (ret < 0) {
		pr_debug("blkdev_issue_discard() failed for WRITE_SAME\n");
		return ret;
	}

	task->task_scsi_status = GOOD;
	transport_complete_task(task, 1);
	return 0;
}

int target_emulate_synchronize_cache(struct se_task *task)
{
	struct se_device *dev = task->task_se_cmd->se_dev;
	struct se_cmd *cmd = task->task_se_cmd;

	if (!dev->transport->do_sync_cache) {
		pr_err("SYNCHRONIZE_CACHE emulation not supported"
			" for: %s\n", dev->transport->name);
		cmd->scsi_sense_reason = TCM_UNSUPPORTED_SCSI_OPCODE;
		return -ENOSYS;
	}

	dev->transport->do_sync_cache(task);
	return 0;
}


#if defined(CONFIG_MACH_QNAPTS)
/* 2014/01/13, support verify(10),(16) for HCK 2.1 (ver:8.100.26063) */

static int __do_v_10_16(
	LIO_SE_CMD *se_cmd, 
	SUBSYSTEM_TYPE type,
	sector_t lba, 
	u32 nr_blks, 
	ERR_REASON_INDEX *err_reason
	)
{
#define ALLOC_BYTES	(1 << 20)
#define TIMEOUT_SEC	(5 * 1000)

	LIO_SE_DEVICE *se_dev = se_cmd->se_dev;
	int ret = -1, bytechk, idx = 0, done_blks = 0;
	u8 *buf = NULL, bs_order;
	u64 data_bytes, alloc_bytes = ALLOC_BYTES, done_bytes = 0, tmp_bytes;
	GEN_RW_TASK r_task;
	struct scatterlist *sg;
	u32 compare_len, tmp_len, tmp_off;

	/**/
	memset((void *)&r_task, 0, sizeof(GEN_RW_TASK));

	bytechk = ((se_cmd->t_task_cdb[1] & 0x06) >> 1);
	if (bytechk == 0x00 || bytechk == 0x02){
		/* sbc3r35j, p173, do nothing here */
		ret = 0;
		goto _EXIT_;
	}

	*err_reason = ERR_ILLEGAL_REQUEST;
	if ((type != SUBSYSTEM_BLOCK) && (type != SUBSYSTEM_FILE))
		goto _EXIT_;

	*err_reason = ERR_OUT_OF_RESOURCES;
	buf = transport_kmap_data_sg(se_cmd);
	if (!buf)
	    goto _EXIT_;

	/* (1) For VBULS bit (verify byte unmapped LBA support bit), it will be
	 * set in target_emulate_evpd_b1(). For now, it will be set ONLY for
	 * thin device
	 *
	 * (2) The case for iblock io + fbdisk disk, actually, we can't do 
	 * anything to get the LBA status (mapped or unmapped)
	 *	 
	 * FIXED ME 
	 *
	 * Here will NOT check the LBA status (mapped or unmapped) while to do
	 * verify. This shall be fixed in the future
	 */
	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	data_bytes = ((u64)nr_blks << bs_order);

	ret = __generic_alloc_sg_list(&alloc_bytes, &r_task.sg_list, 
			&r_task.sg_nents);
	if (ret != 0){
		if (ret == -ENOMEM)
			pr_err("%s - fail to alloc sg list\n", __FUNCTION__);
		if (ret == -EINVAL)
			pr_err("%s - invalid arg during to alloc sg list\n",
				__FUNCTION__);

		*err_reason = ERR_INSUFFICIENT_RESOURCES;
		goto _EXIT_;
	}

	/**/
	while (data_bytes){

		tmp_bytes = min_t(u64, data_bytes, alloc_bytes);

		__make_rw_task(&r_task, se_dev, lba, (tmp_bytes >> bs_order),
			msecs_to_jiffies(TIMEOUT_SEC), DMA_FROM_DEVICE);

		if (type == SUBSYSTEM_BLOCK)
			done_blks = __do_b_rw(&r_task);
		else
			done_blks = __do_f_rw(&r_task);

		/* FIXED ME */
		if (done_blks <= 0 || r_task.is_timeout || r_task.ret_code != 0){
			pr_err("%s: fail to read from source\n", __FUNCTION__);
			*err_reason = ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			goto _EXIT_;
		}

		done_bytes = ((u64)done_blks << bs_order);

		pr_debug("bytchk:0x%x, done_bytes:0x%llx\n", bytechk,
			(unsigned long long)done_bytes);

		/* depend on bytechk to compare data */
		for_each_sg(r_task.sg_list, sg, r_task.sg_nents, idx){

			if (!done_bytes)
				break;

			tmp_len = sg->length;
			tmp_off = 0;

			while (tmp_len){

				if (!done_bytes)
					break;

				/* sbc3r35j, p173 
				 *
				 * bytechk is 0x11b, the data-out buf len is
				 * single logical block
				 */
				if (bytechk == 0x01)
					compare_len = min_t(u32, tmp_len, done_bytes);
				else 
					compare_len = min_t(u32, tmp_len, (1 << bs_order));

				pr_debug("compare_len:0x%x, tmp_off:0x%x\n", 
					compare_len, tmp_off);

				ret = memcmp(buf, sg_virt(sg) + tmp_off, compare_len);
				if (ret)
					goto _EXIT_COMPARE_;

				tmp_len -= compare_len;
				done_bytes -= compare_len;

				if (bytechk == 0x01)
					buf += compare_len;
				else
					tmp_off += compare_len;

			}
		}

_EXIT_COMPARE_:
		if (ret != 0){
			/* FIXED ME */
			pr_err("%s: fail to verify (bytechk:0x%x), lba:0x%llx\n", 
				__FUNCTION__, bytechk, (unsigned long long)lba);
			*err_reason = ERR_MISCOMPARE_DURING_VERIFY_OP;
			goto _EXIT_;
		}

		lba += (sector_t)done_blks;
		data_bytes -= ((u64)done_blks << bs_order);
	}

	ret = 0;

_EXIT_:
	__generic_free_sg_list(r_task.sg_list, r_task.sg_nents);

	if (buf)
		transport_kunmap_data_sg(se_cmd);

	return ret;
}

/* 2014/01/13, support verify(10),(16) for HCK 2.1 (ver:8.100.26063) */

int __verify_10_16(
	LIO_SE_TASK *se_task
	)
{
	LIO_SE_CMD *se_cmd = se_task->task_se_cmd;
	LIO_SE_DEVICE *se_dev = se_cmd->se_dev;
	LIO_FD_DEV *fd_dev = NULL;
	LIO_IBLOCK_DEV *ib_dev = NULL;
	struct inode *inode = NULL;
	int bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	int sector_ret = 0, ret = -EOPNOTSUPP;
	int verify_16_cmd = 0, flush_ret;
	ERR_REASON_INDEX err_reason = ERR_INVALID_CDB_FIELD;
	sector_t lba;
	u32 nr_blks;
	SUBSYSTEM_TYPE type;

	/* FIXED ME, we don't support protection information now ... */
	if (se_cmd->t_task_cdb[1] & 0xe0)
		goto _EXIT_;

	if (se_cmd->t_task_cdb[0] == VERIFY_16)
		verify_16_cmd = 1;

	if (verify_16_cmd){
		nr_blks = __call_transport_get_sectors_16(se_cmd->t_task_cdb, 
			se_cmd, &sector_ret);
		lba =  __call_transport_lba_64(se_cmd->t_task_cdb);
	}
	else {
		nr_blks =  __call_transport_get_sectors_10(se_cmd->t_task_cdb, 
			se_cmd, &sector_ret);
		lba =  __call_transport_lba_32(se_cmd->t_task_cdb);
	}

	/* sbcr35j, p174, if nr blks is zero, it means no any blks shall be
	 * transferred or verified. This condition shall not be considered an error */
	if (!nr_blks){
		ret = 0;
		goto _EXIT_;
	}

	if ((lba + nr_blks - 1) > se_dev->transport->get_blocks(se_dev)){
		pr_err("%s: start lba + verified len exceeds "
			"the capacity size\n", __FUNCTION__);
		err_reason = ERR_LBA_OUT_OF_RANGE;
		goto _EXIT_;
	}

	/* sbc3r35j, p173 */
	if (((se_cmd->t_task_cdb[1] & 0x06) == 0x00)
	|| ((se_cmd->t_task_cdb[1] & 0x06) == 0x04)
	){
		/* sbc3r35j, p173, do nothing here */
		ret = 0;
		goto _EXIT_;
	}

	/* in sbc3r31, p172, before to do verify, we shall write all necessary
	 * data from cache to device first
	 */
	err_reason = ERR_ILLEGAL_REQUEST;
	if(__do_get_subsystem_dev_type(se_dev, &type) != 0)
		goto _EXIT_;

	if (type == SUBSYSTEM_BLOCK){
		ib_dev = (LIO_IBLOCK_DEV *)(se_dev->dev_ptr);
		flush_ret = blkdev_issue_flush(ib_dev->ibd_bd, GFP_KERNEL, NULL);
	}
	else if (type == SUBSYSTEM_FILE){

		fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
		inode = fd_dev->fd_file->f_mapping->host;

		if (!S_ISBLK(inode->i_mode))
			goto _EXIT_;

		flush_ret = blkdev_fsync(fd_dev->fd_file, 
			((loff_t)lba << bs_order), 
			((loff_t)(lba + nr_blks - 1) << bs_order), 0);
	}
	else
		BUG_ON(1);

	if (flush_ret != 0){
		if (type == SUBSYSTEM_BLOCK)
			pr_err("%s: ret:%d after call blkdev_issue_flush()\n", 
				__FUNCTION__, flush_ret);
		else
			pr_err("%s: ret:%d after call blkdev_fsync()\n", 
				__FUNCTION__, flush_ret);
	}
	ret = __do_v_10_16(se_cmd, type, lba, nr_blks, &err_reason);

_EXIT_:
	
	if (ret != 0)
		__set_err_reason(err_reason, &se_cmd->scsi_sense_reason);
	else {
		se_task->task_scsi_status = GOOD;
		transport_complete_task(se_task, 1);
	}

	return ret;
	
}


int target_emulate_logsense(struct se_task *task)
{
    struct se_cmd *cmd = task->task_se_cmd;
    unsigned char *rbuf = NULL;
    int length = 0, ret = -EOPNOTSUPP, i;
    int table_count = ARRAY_SIZE(g_logsense_table);
    unsigned char *buf = NULL;
    u8 pagecode, sub_pagecode;
    ERR_REASON_INDEX err_reason;

    /**/
    err_reason = ERR_PARAMETER_LIST_LEN_ERROR;
    if (!cmd->data_length)
        goto EXIT;

    buf = kmalloc(cmd->data_length, GFP_KERNEL);
    if (!buf)
        goto EXIT;

    rbuf = transport_kmap_data_sg(cmd);
    if (!rbuf)
        goto EXIT;

    memset(buf, 0, cmd->data_length);

    pagecode = (cmd->t_task_cdb[2] & 0x3f);
    sub_pagecode = cmd->t_task_cdb[3];

    for (i = 0; i < table_count; i++){
        if ((g_logsense_table[i].is_end_table == 0x0)
        &&  (g_logsense_table[i].logsense_func != NULL)
        &&  (g_logsense_table[i].page_code == pagecode)
        &&  (g_logsense_table[i].sub_page_code == sub_pagecode)
        )
        {
            length = g_logsense_table[i].logsense_func(cmd, &buf[0]);
            if (length){
                memcpy(rbuf, buf, length);
                task->task_scsi_status = GOOD;
                transport_complete_task(task, 1);
                ret = 0;
            }
            goto EXIT;
        }
    }

    /* if not found any function */
    err_reason = ERR_INVALID_CDB_FIELD;

EXIT:
    if (buf)
        kfree(buf);

    if (rbuf)
        transport_kunmap_data_sg(cmd);

    if (ret != 0)
        __set_err_reason(err_reason, &cmd->scsi_sense_reason);

    return ret;
}




#if defined(SUPPORT_TP)
/* 2014/06/14, adamhsu, redmine 8530
 * Refactor target_emulate_getlbastatus() sicne we will support to get
 * lba mapping status on iSCSI LUN file (i.e. blk i/o + fbdisk and
 * file i/o + static volume)
 */
int target_emulate_getlbastatus(struct se_task *task)
{
#define MAX_DESC_COUNTS		64

	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	unsigned char *buf = NULL;
	sector_t start_lba = get_unaligned_be64(&cmd->t_task_cdb[2]);
	u32 para_data_length = get_unaligned_be32(&cmd->t_task_cdb[10]);
	u32 desc_count;
	int err = (int)MAX_ERR_REASON_INDEX, ret = -EOPNOTSUPP;
	ERR_REASON_INDEX err_reason = MAX_ERR_REASON_INDEX;


	/* minimun value should be 24 = (16 bytes descriptor + 8 bytes) */
	if ( para_data_length < 24 ){
		err_reason = ERR_PARAMETER_LIST_LEN_ERROR;
		goto EXIT;
	}

	if (start_lba > dev->transport->get_blocks(dev)){
		err_reason = ERR_LBA_OUT_OF_RANGE;
		goto EXIT;
	}

	buf = transport_kmap_data_sg(cmd);
	if (!buf){
		err_reason = ERR_INVALID_PARAMETER_LIST;
		goto EXIT;
	}

	if (!cmd->se_dev->transport->do_get_lba_map_status){
		err_reason = ERR_UNKNOWN_SAM_OPCODE;
		goto EXIT;
	}

	/* we have three options ...
	 * (1) block i/o + file-based configuration
	 * (2) file i/o + block-based configuration
	 * (3) file i/o + file-based configuration
	 */
	desc_count = ((para_data_length - 8) >> 4);

	/* In order to reduce the memory allocation failure, we try limit
	 * the descriptor count here. It's ok
	 * sbc3r35j, p114
	 * In response to a GET LBA STATUS command, the device serer may
	 * send less data to Data-In buffer than is specified by allocation len
	 */
	if (desc_count > MAX_DESC_COUNTS)
		desc_count = MAX_DESC_COUNTS;

	ret = cmd->se_dev->transport->do_get_lba_map_status(
		cmd, start_lba, desc_count, buf, &err);

EXIT:
	if (buf)
		transport_kunmap_data_sg(cmd);
	
	if (ret != 0){
		if (err != (int)MAX_ERR_REASON_INDEX)
			err_reason = (ERR_REASON_INDEX)err;
		else
			err_reason = ERR_UNKNOWN_SAM_OPCODE;

		__set_err_reason(err_reason, &cmd->scsi_sense_reason);
	} else {
		task->task_scsi_status = GOOD;
		transport_complete_task(task, 1);
	}
	    
	return ret;

}

#endif /* defined(SUPPORT_TP) */
#endif /* defined(CONFIG_MACH_QNAPTS) */


int target_emulate_noop(struct se_task *task)
{
	task->task_scsi_status = GOOD;
	transport_complete_task(task, 1);
	return 0;
}

/*
 * Write a CDB into @cdb that is based on the one the intiator sent us,
 * but updated to only cover the sectors that the current task handles.
 */
void target_get_task_cdb(struct se_task *task, unsigned char *cdb)
{
	struct se_cmd *cmd = task->task_se_cmd;
	unsigned int cdb_len = scsi_command_size(cmd->t_task_cdb);

	memcpy(cdb, cmd->t_task_cdb, cdb_len);
	if (cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB) {
		unsigned long long lba = task->task_lba;
		u32 sectors = task->task_sectors;

		switch (cdb_len) {
		case 6:
			/* 21-bit LBA and 8-bit sectors */
			cdb[1] = (lba >> 16) & 0x1f;
			cdb[2] = (lba >> 8) & 0xff;
			cdb[3] = lba & 0xff;
			cdb[4] = sectors & 0xff;
			break;
		case 10:
			/* 32-bit LBA and 16-bit sectors */
			put_unaligned_be32(lba, &cdb[2]);
			put_unaligned_be16(sectors, &cdb[7]);
			break;
		case 12:
			/* 32-bit LBA and 32-bit sectors */
			put_unaligned_be32(lba, &cdb[2]);
			put_unaligned_be32(sectors, &cdb[6]);
			break;
		case 16:
			/* 64-bit LBA and 32-bit sectors */
			put_unaligned_be64(lba, &cdb[2]);
			put_unaligned_be32(sectors, &cdb[10]);
			break;
		case 32:
			/* 64-bit LBA and 32-bit sectors, extended CDB */
			put_unaligned_be64(lba, &cdb[12]);
			put_unaligned_be32(sectors, &cdb[28]);
			break;
		default:
			BUG();
		}
	}
}
EXPORT_SYMBOL(target_get_task_cdb);

#if defined(CONFIG_MACH_QNAPTS)
//Benjamin 20121105 sync VAAI support from Adam. 

/*
 * @fn void __make_target_naa_6h_hdr(LIO_SE_DEVICE *se_dev, u8 *buf)
 * @brief Simple function to make the header of NAA
 *
 * @sa
 * @param[in] se_dev
 * @param[in] byte[0-3] will be put the header data
 * @retval NA
 */
void __make_target_naa_6h_hdr(
	LIO_SE_DEVICE *se_dev, 
	u8 *buf
	)
{
	if(!strcmp(se_dev->se_sub_dev->se_dev_naa, "qnap")) {
		buf[0] = (0x6 << 4)| 0x0e;
		buf[1] = 0x84;
		buf[2] = 0x3b;
		buf[3] = (0x6 << 4);
	}else{
		buf[0] = (0x6 << 4);
		buf[1] = 0x01;
		buf[2] = 0x40;
		buf[3] = (0x5 << 4);
	}
	return;
}


/*
 * @fn void __get_target_parse_naa_6h_vendor_specific(IN LIO_SE_DEVICE *pSeDev, IN u8 *pu8Buf)
 * @brief Parse / make the string for NAA
 *
 * @sa
 * @param[in] pDev
 * @param[in] pu8Buf Buffer which will be put the NAA string.
 * @retval NA
 */
void __get_target_parse_naa_6h_vendor_specific(
	IN LIO_SE_DEVICE *pSeDev, 
	IN u8 *pu8Buf
	)
{
	target_parse_naa_6h_vendor_specific(pSeDev, pu8Buf);
	return;
}
#endif


