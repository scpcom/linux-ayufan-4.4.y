#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <asm/types.h>
#include "i2c-mv.h"
#include "../rtc/rtc.h"

#ifdef CONFIG_MACH_SYNOLOGY_6281
#include <linux/i2c.h>

int mvI2CCharWrite(int target, u8 *data, int length, int offset)
{
	struct i2c_adapter *pAdap = NULL;
	struct i2c_msg msg = {0};
	int ret = -1;
	int buf_len = 0;
	u8 *pDataBuf = NULL;

	pAdap = i2c_get_adapter(0);
	if (!pAdap) {
		printk("Cannot get i2c adapter\n");
		goto END;
	}

	if (-1 == offset) {
		buf_len = length;
	} else {
		buf_len = length + 1;
	}

	if (NULL == (pDataBuf = kzalloc(buf_len, GFP_KERNEL))){
		printk("error malloc\n");
		goto END;
	}

	msg.addr = target;
	msg.flags = 0;
	msg.len = buf_len;
	msg.buf = pDataBuf;

	if (-1 == offset) {
		memcpy(pDataBuf, data, length);
	} else {
		pDataBuf[0] = offset << 4;
		memcpy(pDataBuf + 1, data, length);
	}

	if (1 != i2c_transfer(pAdap, &msg, 1)) {
		goto END;
	}

	ret = 0;
END:
	if (pAdap)
		i2c_put_adapter(pAdap);

	if (pDataBuf)
	    kfree(pDataBuf);

	return ret;
}

int mvI2CCharRead(int target, u8 *data, int length, int offset)
{
	struct i2c_adapter *pAdap;
	struct i2c_msg msg;
	int ret = -1;
	u8 offset_u8 = offset;
	char *pDataBuf = NULL;
	int real_length;

	pAdap = i2c_get_adapter(0);
	if (!pAdap) {
		printk("Cannot get i2c adapter\n");
		goto END;
	}

	if (-1 == offset) {
		offset_u8 = 0;
	}

	if (-1 == offset) {
		real_length = length;
	} else {
		real_length = 17;
	}

	if (NULL == (pDataBuf = kzalloc(real_length, GFP_KERNEL))){
		printk("error malloc\n");
		goto END;
	}

	msg.addr = target;
	msg.flags = I2C_M_RD;
	msg.buf = pDataBuf;
	msg.len = real_length;

	if (1 != i2c_transfer(pAdap, &msg, 1))
		goto END;

	if (-1 != offset) {
		memcpy(data, pDataBuf + 1 + offset, length);
	} else {
		memcpy(data, pDataBuf, length);
	}

	ret = 0;
END:
	if (pAdap)
		i2c_put_adapter(pAdap);

	if (pDataBuf)
		kfree(pDataBuf);

	return ret;
}
#else
/* The TWSI interface supports both 7-bit and 10-bit addressing.
 * This enumerator describes addressing type.
 */
typedef enum _mvTwsiAddrType
{
        ADDR7_BIT,                      /* 7 bit address    */
        ADDR10_BIT                      /* 10 bit address   */
}MV_TWSI_ADDR_TYPE;

/* This structure describes TWSI address. */
typedef struct _mvTwsiAddr
{
        u32 address;                    /* address          */
        MV_TWSI_ADDR_TYPE type;       /* Address type     */
}MV_TWSI_ADDR;

/* This structure describes a TWSI slave. */
typedef struct _mvTwsiSlave
{
        MV_TWSI_ADDR slaveAddr;
        int validOffset;    /* whether the slave has offset (i.e. Eeprom  etc.) */
        u32 offset;         /* offset in the slave. */
        int moreThen256;    /* whether the ofset is bigger then 256 */
}MV_TWSI_SLAVE;

/******************************************************************************
 * Marvell 88F5182 i2c control                                                *
 *                                                                            *
 * Those structures are copied from arch/arm/mach-mv88fxx81/Soc/twsi/mvTwsi.h *
 * ***************************************************************************/

/********************************************************************************
 * Marvell 88F6281 i2c control                                                  *
 *                                                                              *
 * Those structures are copied from arch/arm/plat-feroceon/mv_hal/twsi/mvTwsi.h *
 * *****************************************************************************/

#ifdef CONFIG_SYNO_MV88F5x8x
extern int mvTwsiRead (MV_TWSI_SLAVE *twsiSlave, u8 *pBlock, u32 blockSize);
extern int mvTwsiWrite(MV_TWSI_SLAVE *twsiSlave, u8 *pBlock, u32 blockSize);
#else
extern int mvTwsiRead (char chanNum, MV_TWSI_SLAVE *twsiSlave, u8 *pBlock, u32 blockSize);
extern int mvTwsiWrite(char chanNum, MV_TWSI_SLAVE *twsiSlave, u8 *pBlock, u32 blockSize);
#endif

int mvI2CCharRead(int target, u8 *data, int length, int offset)
{
	MV_TWSI_SLAVE   twsiSlave;
	
	twsiSlave.slaveAddr.type = ADDR7_BIT;
	twsiSlave.slaveAddr.address = target;
	twsiSlave.validOffset = (offset >= 0) ? 1 : 0;
	twsiSlave.offset = (offset<<4);
	twsiSlave.moreThen256 = 0;
#ifdef CONFIG_SYNO_MV88F5x8x
	return mvTwsiRead (&twsiSlave, data, length);
#elif MY_ABC_HERE
	return mvTwsiRead (0, &twsiSlave, data, length);
#endif
}

int mvI2CCharWrite(int target, u8 *data, int length, int offset)
{
	MV_TWSI_SLAVE twsiSlave;
	
	twsiSlave.slaveAddr.type = ADDR7_BIT;
	twsiSlave.slaveAddr.address = target;
	twsiSlave.validOffset = (offset >= 0) ? 1 : 0;
	twsiSlave.offset = (offset<<4);
	twsiSlave.moreThen256 = 0;
#ifdef CONFIG_SYNO_MV88F5x8x
	return mvTwsiWrite(&twsiSlave, data, length);
#elif MY_ABC_HERE
	return mvTwsiWrite(0, &twsiSlave, data, length);
#endif
}
#endif
