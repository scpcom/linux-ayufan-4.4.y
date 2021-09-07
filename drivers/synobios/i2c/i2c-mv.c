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
 
typedef enum _mvTwsiAddrType
{
        ADDR7_BIT,                       
        ADDR10_BIT                       
}MV_TWSI_ADDR_TYPE;

typedef struct _mvTwsiAddr
{
        u32 address;                     
        MV_TWSI_ADDR_TYPE type;        
}MV_TWSI_ADDR;

typedef struct _mvTwsiSlave
{
        MV_TWSI_ADDR slaveAddr;
        int validOffset;     
        u32 offset;          
        int moreThen256;     
}MV_TWSI_SLAVE;

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
