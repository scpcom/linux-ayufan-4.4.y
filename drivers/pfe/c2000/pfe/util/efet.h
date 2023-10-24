#ifndef _UTIL_EFET_H_
#define _UTIL_EFET_H_

#define EFET_ENTRY_ADDR		0x00
#define EFET_ENTRY_SIZE		0x04
#define EFET_ENTRY_DMEM_ADDR	0x08
#define EFET_ENTRY_STATUS	0x0c
#define EFET_ENTRY_ENDIAN	0x10

#define CBUS2DMEM	0
#define DMEM2CBUS	1

#define EFET2BUS_LE     (1 << 0)

#define EFET1		0
#define EFET2		1
#define EFET3		2

static u32 util_efet_status;
static const unsigned long util_efet_baseaddr[3] = {EFET1_BASE_ADDR, EFET2_BASE_ADDR, EFET3_BASE_ADDR};

#define UTIL_EFET(i, cbus_addr, dmem_addr,len,dir) do { \
	__writel((len & 0x3FF) | (dir << 16), util_efet_baseaddr[i] + EFET_ENTRY_SIZE); \
	__writel(dmem_addr, util_efet_baseaddr[i] + EFET_ENTRY_DMEM_ADDR);\
	__writel(cbus_addr, util_efet_baseaddr[i] + EFET_ENTRY_ADDR);\
	nop();\
	}while(0)

/** Checks if the util efet as completed the last transaction.
* Can be called at any time.
*
* @param i      Efet index
*
* @return       0 - efet busy, 1 - efet completed.
*
*/
static inline int util_efet_complete(int i)
{
        if (!(readl(util_efet_baseaddr[i] + EFET_ENTRY_STATUS) & 0x1))
                return 0;

        util_efet_status &= ~(1 << i);

        return 1;
}

/** Waits for the util efet to finish a transaction, blocking the caller.
* Can be called at any time.
*
* @param i      Efet index
*
*/
static inline void util_efet_wait(int i)
{
        while(1)
        {
                if (util_efet_complete(i))
                        break;

        }
}

/** Asynchronous interface to util efet read/write functions.
* It will wait for the efet to finish previous transaction, but does not wait for the current transaction to finish.
*
* @param i              Efet index
* @param cbus_addr      Cbus address (must be 64bits aligned)
* @param dmem_addr      DMEM address (must be 64bits aligned)
* @param len            Number of bytes to copy (must be 64bits aligned size)
* @param dir            Direction of the transaction (0 - cbus to dmem, 1 - dmem to cbus)
*
*/
static inline void util_efet_async(int i, u32 cbus_addr, u32 dmem_addr, u32 len, u8 dir)
{
        if (util_efet_status & (1 << i))
                util_efet_wait(i);

        UTIL_EFET(i, cbus_addr, dmem_addr, len, dir);
	util_efet_status |= (1 << 1);	
}


static inline void util_efet_async0( u32 cbus_addr, u32 dmem_addr, u32 len, u8 dir)
{
	util_efet_async(0,cbus_addr,dmem_addr,len,dir);
}

/* EFET 2 is aways used for SYNC operations */
static inline void util_efet_sync2( u32 cbus_addr, u32 dmem_addr, u32 len, u8 dir)
{
	UTIL_EFET(2,cbus_addr,dmem_addr,len,dir);
	while (!(readl(util_efet_baseaddr[2] + EFET_ENTRY_STATUS) & 0x1));

	return;
}

void util_efet_sync0( u32 cbus_addr, u32 dmem_addr, u32 len, u8 dir);
#endif /* _UTIL_EFET_H_ */

