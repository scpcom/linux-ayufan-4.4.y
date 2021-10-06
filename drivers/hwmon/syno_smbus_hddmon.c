/* Copyright (c) 2000-2019 Synology Inc. All rights reserved. */
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/syno.h>
#include <linux/synobios.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#define BPCPLD_ADAPTER 0
#define BPCPLD_TYPE "gowin"
#define BPCPLD_LOCATION 0x42
#define BPCPLD_HDD_PRESENT 0x20
#define BPCPLD_HDD_ENABLE 0x10

typedef struct SynoSMBusHddMonData {
    int HddPresentStat;
    int HddEnableStat;
    //if need spinup group, add spinup group & delay variable here
} SynoSMBusHddMonData_t;

static struct i2c_client *gpClient;
static SynoSMBusHddMonData_t synoSMBusHddMonData;

struct i2c_board_info __initdata CPLDI2CBoardInfo[] = {
	{
		I2C_BOARD_INFO(BPCPLD_TYPE, BPCPLD_LOCATION),
	},
};

static int syno_smbus_hddmon_data_init(SynoSMBusHddMonData_t *pData)
{
	int iRet = -1;

	if (NULL == pData) {
		goto END;
	}

	memset(pData, 0, sizeof(SynoSMBusHddMonData_t));

//if need spinup group, get spinup group & delay info here

	iRet = 0;
END:
	return iRet;
}

static int syno_smbus_set_hdd_enable_data(int val)
{
	return i2c_smbus_write_byte_data(gpClient, BPCPLD_HDD_ENABLE, val);
}

static int syno_i2c_client_init(void)
{
	int iRet = -1;
	struct i2c_adapter *pAdapter = NULL;

	/* instantiate the devices explicitly */
	pAdapter = i2c_get_adapter(BPCPLD_ADAPTER);
	if (NULL == pAdapter) {
		printk(KERN_ERR "BP CPLD initial error: failed to get i2c adapter\n");
		goto END;
	}

	/*regist board info*/
	gpClient = i2c_new_device(pAdapter, &CPLDI2CBoardInfo[0]);
	if (NULL == gpClient) {
		printk(KERN_ERR "BP CPLD initial error: failed to initial device\n");
		goto END;
	}

	iRet = 0;

END:
	if (pAdapter) {
		i2c_put_adapter(pAdapter);
	}

	return iRet;
}

static int syno_smbus_hdd_enable_spinup(SynoSMBusHddMonData_t *pData)
{
	int iRet = -1;
	int iEnableVal;

	/* for RS1220+, directly set enable data byte to 0xff.
	 * if need spinup group, add code here.
	 */
	iEnableVal = 0xff;
	iRet = syno_smbus_set_hdd_enable_data(iEnableVal);
	if (0 > iRet) {
		goto END;
	}

END:
	return iRet;
}

static int __init syno_smbus_hddmon_init(void)
{
	int iRet = -1;

    iRet = syno_smbus_hddmon_data_init(&synoSMBusHddMonData);
	if (0 > iRet) {
		goto END;
	}

	iRet = syno_i2c_client_init();
	if (0 > iRet) {
		goto END;
	}

	iRet = syno_smbus_hdd_enable_spinup(&synoSMBusHddMonData);
	if (0 > iRet) {
		goto END;
	}

	printk("Syno_SMBus_HddMon: Initialization completed.\n");

	iRet = 0;
END:
	return iRet;
}

static void __exit syno_smbus_hddmon_exit(void)
{
	i2c_unregister_device(gpClient);
	printk("Syno_SMBus_HddMon: Exit.\n");
}

MODULE_AUTHOR("Chih-Chien Chien");
MODULE_DESCRIPTION("Syno_SMBus_HddMon\n");
MODULE_LICENSE("GPL");

module_init(syno_smbus_hddmon_init);
module_exit(syno_smbus_hddmon_exit);
