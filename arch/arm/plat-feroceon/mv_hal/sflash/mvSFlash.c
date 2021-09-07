#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvOs.h"
#include "sflash/mvSFlash.h"
#include "sflash/mvSFlashSpec.h"
#include "spi/mvSpi.h"
#include "spi/mvSpiCmnd.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

static MV_SFLASH_DEVICE_PARAMS sflash[] = {
#ifdef MY_ABC_HERE
	 
	{
	 MV_S25FL_WREN_CMND_OPCD,
	 MV_S25FL_WRDI_CMND_OPCD,
	 MV_S25FL_RDID_CMND_OPCD,
	 MV_S25FL_RDSR_CMND_OPCD,
	 MV_S25FL_WRSR_CMND_OPCD,
	 MV_S25FL_READ_CMND_OPCD,
	 MV_S25FL_FAST_RD_CMND_OPCD,
	 MV_S25FL_PP_CMND_OPCD,
	 MV_S25FL_SE_CMND_OPCD,
	 MV_S25FL_BE_CMND_OPCD,
	 MV_S25FL_RES_CMND_OPCD,
	 MV_SFLASH_NO_SPECIFIC_OPCD,
	 MV_S25FL032A_SECTOR_SIZE,
	 MV_S25FL032A_SECTOR_NUMBER,
	 MV_S25FL_PAGE_SIZE,
	 "SPANSION S25FL032A",
	 MV_SPANSION_MANF_ID,
	 MV_S25FL032A_DEVICE_ID,
	 MV_S25FL032A_MAX_SPI_FREQ,
	 MV_S25FL032A_MAX_FAST_SPI_FREQ,
	 MV_S25FL032A_FAST_READ_DUMMY_BYTES
	},
#endif
     
    {
     MV_M25P_WREN_CMND_OPCD,
     MV_M25P_WRDI_CMND_OPCD,
     MV_M25P_RDID_CMND_OPCD,
     MV_M25P_RDSR_CMND_OPCD,
     MV_M25P_WRSR_CMND_OPCD,
     MV_M25P_READ_CMND_OPCD,
     MV_M25P_FAST_RD_CMND_OPCD,
     MV_M25P_PP_CMND_OPCD,
     MV_M25P_SE_CMND_OPCD,
     MV_M25P_BE_CMND_OPCD,
     MV_M25P_RES_CMND_OPCD,
     MV_SFLASH_NO_SPECIFIC_OPCD,     
     MV_M25P32_SECTOR_SIZE,
     MV_M25P32_SECTOR_NUMBER,
     MV_M25P_PAGE_SIZE,
     "ST M25P32",
     MV_M25PXXX_ST_MANF_ID,
     MV_M25P32_DEVICE_ID,
     MV_M25P32_MAX_SPI_FREQ,
     MV_M25P32_MAX_FAST_SPI_FREQ,
     MV_M25P32_FAST_READ_DUMMY_BYTES
    },
     
    {
     MV_M25P_WREN_CMND_OPCD,
     MV_M25P_WRDI_CMND_OPCD,
     MV_M25P_RDID_CMND_OPCD,
     MV_M25P_RDSR_CMND_OPCD,
     MV_M25P_WRSR_CMND_OPCD,
     MV_M25P_READ_CMND_OPCD,
     MV_M25P_FAST_RD_CMND_OPCD,
     MV_M25P_PP_CMND_OPCD,
     MV_M25P_SE_CMND_OPCD,
     MV_M25P_BE_CMND_OPCD,
     MV_M25P_RES_CMND_OPCD,
     MV_SFLASH_NO_SPECIFIC_OPCD,     
     MV_M25P64_SECTOR_SIZE,
     MV_M25P64_SECTOR_NUMBER,
     MV_M25P_PAGE_SIZE,
     "ST M25P64",
     MV_M25PXXX_ST_MANF_ID,
     MV_M25P64_DEVICE_ID,
     MV_M25P64_MAX_SPI_FREQ,
     MV_M25P64_MAX_FAST_SPI_FREQ,
     MV_M25P64_FAST_READ_DUMMY_BYTES
    },
     
    {
     MV_M25P_WREN_CMND_OPCD,
     MV_M25P_WRDI_CMND_OPCD,
     MV_M25P_RDID_CMND_OPCD,
     MV_M25P_RDSR_CMND_OPCD,
     MV_M25P_WRSR_CMND_OPCD,
     MV_M25P_READ_CMND_OPCD,
     MV_M25P_FAST_RD_CMND_OPCD,
     MV_M25P_PP_CMND_OPCD,
     MV_M25P_SE_CMND_OPCD,
     MV_M25P_BE_CMND_OPCD,
     MV_M25P_RES_CMND_OPCD,
     MV_SFLASH_NO_SPECIFIC_OPCD,     
     MV_M25P128_SECTOR_SIZE,
     MV_M25P128_SECTOR_NUMBER,
     MV_M25P_PAGE_SIZE,
     "ST M25P128",
     MV_M25PXXX_ST_MANF_ID,
     MV_M25P128_DEVICE_ID,
     MV_M25P128_MAX_SPI_FREQ,
     MV_M25P128_MAX_FAST_SPI_FREQ,
     MV_M25P128_FAST_READ_DUMMY_BYTES
    },
#ifdef MY_ABC_HERE
	  
	{
	MV_N25Q_WREN_CMND_OPCD,
	MV_N25Q_WRDI_CMND_OPCD,
	MV_N25Q_RDID_CMND_OPCD,
	MV_N25Q_RDSR_CMND_OPCD,
	MV_N25Q_WRSR_CMND_OPCD,
	MV_N25Q_READ_CMND_OPCD,
	MV_N25Q_FAST_RD_CMND_OPCD,
	MV_N25Q_PP_CMND_OPCD,
	MV_N25Q_SE_CMND_OPCD,
    MV_N25Q_BE_CMND_OPCD,
	MV_SFLASH_NO_SPECIFIC_OPCD,     
	MV_SFLASH_NO_SPECIFIC_OPCD,     
    MV_N25Q032_SECTOR_SIZE,
    MV_N25Q032_SECTOR_NUMBER,
    MV_N25Q_PAGE_SIZE,
	"ST N25Q032",
	MV_N25QXXX_ST_MANF_ID,
	MV_N25Q032_DEVICE_ID,
	MV_N25Q032_MAX_SPI_FREQ,
	MV_N25Q032_MAX_FAST_SPI_FREQ,
	MV_N25Q032_FAST_READ_DUMMY_BYTES
	},
	 
	{
    MV_MX25L_WREN_CMND_OPCD,
    MV_MX25L_WRDI_CMND_OPCD,
	MV_MX25L_RDID_CMND_OPCD,
	MV_MX25L_RDSR_CMND_OPCD,
	MV_MX25L_WRSR_CMND_OPCD,
	MV_MX25L_READ_CMND_OPCD,
	MV_MX25L_FAST_RD_CMND_OPCD,
	MV_MX25L_PP_CMND_OPCD,
	MV_MX25L_SE_CMND_OPCD,
	MV_MX25L_BE_CMND_OPCD,
	MV_MX25L_RES_CMND_OPCD,
	MV_MX25L_DP_CMND_OPCD,
	MV_MX25L3206_SECTOR_SIZE,
	MV_MX25L3206_SECTOR_NUMBER,
	MV_MXIC_PAGE_SIZE,
	"MXIC MX25L3206E",
	MV_MXIC_MANF_ID,
	MV_MX25L3206_DEVICE_ID,
	MV_MX25L3206_MAX_SPI_FREQ,
	MV_MX25L3206_MAX_FAST_SPI_FREQ,
	MV_MX25L3206_FAST_READ_DUMMY_BYTES
    },
	 
	{
	MV_N25Q_WREN_CMND_OPCD,
	MV_N25Q_WRDI_CMND_OPCD,
	MV_N25Q_RDID_CMND_OPCD,
	MV_N25Q_RDSR_CMND_OPCD,
	MV_N25Q_WRSR_CMND_OPCD,
	MV_N25Q_READ_CMND_OPCD,
	MV_N25Q_FAST_RD_CMND_OPCD,
	MV_N25Q_PP_CMND_OPCD,
	MV_N25Q_SE_CMND_OPCD,
	MV_N25Q_BE_CMND_OPCD,
	MV_SFLASH_NO_SPECIFIC_OPCD,     
	MV_SFLASH_NO_SPECIFIC_OPCD,     
	MV_N25Q064_SECTOR_SIZE,
	MV_N25Q064_SECTOR_NUMBER,
	MV_N25Q_PAGE_SIZE,
	"ST N25Q064",
	MV_N25QXXX_ST_MANF_ID,
	MV_N25Q064_DEVICE_ID,
	MV_N25Q064_MAX_SPI_FREQ,
	MV_N25Q064_MAX_FAST_SPI_FREQ,
	MV_N25Q064_FAST_READ_DUMMY_BYTES
	},
#endif
     
    {
     MV_MX25L_WREN_CMND_OPCD,
     MV_MX25L_WRDI_CMND_OPCD,
     MV_MX25L_RDID_CMND_OPCD,
     MV_MX25L_RDSR_CMND_OPCD,
     MV_MX25L_WRSR_CMND_OPCD,
     MV_MX25L_READ_CMND_OPCD,
     MV_MX25L_FAST_RD_CMND_OPCD,
     MV_MX25L_PP_CMND_OPCD,
     MV_MX25L_SE_CMND_OPCD,
     MV_MX25L_BE_CMND_OPCD,
     MV_MX25L_RES_CMND_OPCD,
     MV_MX25L_DP_CMND_OPCD,
     MV_MX25L6405_SECTOR_SIZE,
     MV_MX25L6405_SECTOR_NUMBER,
     MV_MXIC_PAGE_SIZE,
#ifdef MY_ABC_HERE
     
     "MXIC MX25L6406E/MX25L6445EM2I-10G",
#else
	 "MXIC MX25L6405",
#endif
     MV_MXIC_MANF_ID,
     MV_MX25L6405_DEVICE_ID,
     MV_MX25L6405_MAX_SPI_FREQ,
     MV_MX25L6405_MAX_FAST_SPI_FREQ,
     MV_MX25L6405_FAST_READ_DUMMY_BYTES
    },
#ifdef MY_ABC_HERE
     
    {
     MV_S25FL_WREN_CMND_OPCD,
     MV_S25FL_WRDI_CMND_OPCD,
     MV_S25FL_RDID_CMND_OPCD,
     MV_S25FL_RDSR_CMND_OPCD,
     MV_S25FL_WRSR_CMND_OPCD,
     MV_S25FL_READ_CMND_OPCD,
     MV_S25FL_FAST_RD_CMND_OPCD,
     MV_S25FL_PP_CMND_OPCD,
     MV_S25FL_SE_CMND_OPCD,
     MV_S25FL_BE_CMND_OPCD,
     MV_S25FL_RES_CMND_OPCD,
     MV_S25FL_DP_CMND_OPCD,
     MV_S25FL064_SECTOR_SIZE,
     MV_S25FL064_SECTOR_NUMBER,
     MV_S25FL_PAGE_SIZE,
     "SPANSION S25FL064",
     MV_SPANSION_MANF_ID,
     MV_S25FL064_DEVICE_ID,
     MV_S25FL064_MAX_SPI_FREQ,
     MV_S25FL064_MAX_FAST_SPI_FREQ,
     MV_S25FL064_FAST_READ_DUMMY_BYTES
    },
#endif
     
    {
     MV_S25FL_WREN_CMND_OPCD,
     MV_S25FL_WRDI_CMND_OPCD,
     MV_S25FL_RDID_CMND_OPCD,
     MV_S25FL_RDSR_CMND_OPCD,
     MV_S25FL_WRSR_CMND_OPCD,
     MV_S25FL_READ_CMND_OPCD,
     MV_S25FL_FAST_RD_CMND_OPCD,
     MV_S25FL_PP_CMND_OPCD,
     MV_S25FL_SE_CMND_OPCD,
     MV_S25FL_BE_CMND_OPCD,
     MV_S25FL_RES_CMND_OPCD,
     MV_S25FL_DP_CMND_OPCD,
     MV_S25FL128_SECTOR_SIZE,
     MV_S25FL128_SECTOR_NUMBER,
     MV_S25FL_PAGE_SIZE,
     "SPANSION S25FL128",
     MV_SPANSION_MANF_ID,
     MV_S25FL128_DEVICE_ID,
     MV_S25FL128_MAX_SPI_FREQ,
     MV_M25P128_MAX_FAST_SPI_FREQ,
     MV_M25P128_FAST_READ_DUMMY_BYTES
    }
#ifdef MY_ABC_HERE
    ,
     
   {
     MV_SST25VF_WREN_CMND_OPCD,
     MV_SST25VF_WRDI_CMND_OPCD,
     MV_SST25VF_RDID_CMND_OPCD,
     MV_SST25VF_RDSR_CMND_OPCD,
     MV_SST25VF_WRSR_CMND_OPCD,
     MV_SST25VF_READ_CMND_OPCD,
     MV_SST25VF_FAST_RD_CMND_OPCD,
     MV_SST25VF_PP_CMND_OPCD,
     MV_SST25VF_SE_CMND_OPCD,
     MV_SST25VF_BE_CMND_OPCD,
     MV_SST25VF_RES_CMND_OPCD,
     MV_SFLASH_NO_SPECIFIC_OPCD,
     MV_SST25VF032_SECTOR_SIZE,
     MV_SST25VF032_SECTOR_NUMBER,
     MV_SST_PAGE_SIZE,
     "SST SST25VF032",
     MV_SST_MANF_ID,
     MV_SST25VF032_DEVICE_ID,
     MV_SST25VF032_MAX_SPI_FREQ,
     MV_SST25VF032_MAX_FAST_SPI_FREQ,
     MV_SST25VF032_FAST_READ_DUMMY_BYTES
    }
#endif
};

static MV_STATUS    mvWriteEnable   (MV_SFLASH_INFO * pFlinfo);
static MV_STATUS    mvStatusRegGet  (MV_SFLASH_INFO * pFlinfo, MV_U8 * pStatReg);
static MV_STATUS    mvStatusRegSet  (MV_SFLASH_INFO * pFlinfo, MV_U8 sr);
static MV_STATUS    mvWaitOnWipClear(MV_SFLASH_INFO * pFlinfo);
static MV_STATUS    mvSFlashPageWr  (MV_SFLASH_INFO * pFlinfo, MV_U32 offset, \
							         MV_U8* pPageBuff, MV_U32 buffSize);
static MV_STATUS    mvSFlashWithDefaultsIdGet (MV_SFLASH_INFO * pFlinfo, \
                                            MV_U8* manId, MV_U16* devId);

static MV_STATUS mvWriteEnable(MV_SFLASH_INFO * pFlinfo)
{
	MV_U8 cmd[MV_SFLASH_WREN_CMND_LENGTH];

    cmd[0] = sflash[pFlinfo->index].opcdWREN;

	return mvSpiWriteThenRead(cmd, MV_SFLASH_WREN_CMND_LENGTH, NULL, 0, 0);
}

static MV_STATUS mvStatusRegGet(MV_SFLASH_INFO * pFlinfo, MV_U8 * pStatReg)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_RDSR_CMND_LENGTH];
	MV_U8 sr[MV_SFLASH_RDSR_REPLY_LENGTH];

	cmd[0] = sflash[pFlinfo->index].opcdRDSR;

	if ((ret = mvSpiWriteThenRead(cmd, MV_SFLASH_RDSR_CMND_LENGTH, sr,
                                         MV_SFLASH_RDSR_REPLY_LENGTH,0)) != MV_OK)
        return ret;

    *pStatReg = sr[0];

    return MV_OK;
}

static MV_STATUS mvWaitOnWipClear(MV_SFLASH_INFO * pFlinfo)
{
    MV_STATUS ret;
	MV_U32 i;
    MV_U8 stat;

	for (i=0; i<MV_SFLASH_MAX_WAIT_LOOP; i++)
	{
        if ((ret = mvStatusRegGet(pFlinfo, &stat)) != MV_OK)
            return ret;

		if ((stat & MV_SFLASH_STATUS_REG_WIP_MASK) == 0)
			return MV_OK;
	}

    DB(mvOsPrintf("%s WARNING: Write Timeout!\n", __FUNCTION__);)
	return MV_TIMEOUT;
}

static MV_STATUS mvWaitOnChipEraseDone(MV_SFLASH_INFO * pFlinfo)
{
    MV_STATUS ret;
	MV_U32 i;
    MV_U8 stat;

	for (i=0; i<MV_SFLASH_CHIP_ERASE_MAX_WAIT_LOOP; i++)
	{
        if ((ret = mvStatusRegGet(pFlinfo, &stat)) != MV_OK)
            return ret;

		if ((stat & MV_SFLASH_STATUS_REG_WIP_MASK) == 0)
			return MV_OK;
	}

    DB(mvOsPrintf("%s WARNING: Write Timeout!\n", __FUNCTION__);)
	return MV_TIMEOUT;
}

static MV_STATUS mvStatusRegSet(MV_SFLASH_INFO * pFlinfo, MV_U8 sr)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_WRSR_CMND_LENGTH];

	if ((ret = mvWriteEnable(pFlinfo)) != MV_OK)
		return ret;

    cmd[0] = sflash[pFlinfo->index].opcdWRSR;
	cmd[1] = sr;

	if ((ret = mvSpiWriteThenRead(cmd, MV_SFLASH_WRSR_CMND_LENGTH, NULL, 0, 0)) != MV_OK)
		return ret;

    if ((ret = mvWaitOnWipClear(pFlinfo)) != MV_OK)
		return ret;

    mvOsDelay(1);

    return MV_OK;
}

static MV_STATUS mvSFlashPageWr (MV_SFLASH_INFO * pFlinfo, MV_U32 offset,
							     MV_U8* pPageBuff, MV_U32 buffSize)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_PP_CMND_LENGTH];

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invalid parameter device index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if (((offset & (sflash[pFlinfo->index].pageSize - 1)) + buffSize) >
        sflash[pFlinfo->index].pageSize)
    {
        DB(mvOsPrintf("%s WARNING: Page allignment problem!\n", __FUNCTION__);)
		return MV_OUT_OF_RANGE;
    }

	if ((ret = mvWriteEnable(pFlinfo)) != MV_OK)
		return ret;

    cmd[0] = sflash[pFlinfo->index].opcdPP;
	cmd[1] = ((offset >> 16) & 0xFF);
	cmd[2] = ((offset >> 8) & 0xFF);
	cmd[3] = (offset & 0xFF);

	if ((ret = mvSpiWriteThenWrite(cmd, MV_SFLASH_PP_CMND_LENGTH, pPageBuff, buffSize)) != MV_OK)
		return ret;

	if ((ret = mvWaitOnWipClear(pFlinfo)) != MV_OK)
		return ret;

	return MV_OK;
}

static MV_STATUS mvSFlashWithDefaultsIdGet (MV_SFLASH_INFO * pFlinfo, MV_U8* manId, MV_U16* devId)
{
    MV_STATUS ret;
    MV_U8 cmdRDID[MV_SFLASH_RDID_CMND_LENGTH];
	MV_U8 id[MV_SFLASH_RDID_REPLY_LENGTH];

    cmdRDID[0] = MV_SFLASH_DEFAULT_RDID_OPCD;    
	if ((ret = mvSpiWriteThenRead(cmdRDID, MV_SFLASH_RDID_CMND_LENGTH, id, MV_SFLASH_RDID_REPLY_LENGTH, 0)) != MV_OK)
		return ret;

	*manId = id[0];
	*devId = 0;
	*devId |= (id[1] << 8);
	*devId |= id[2];

	return MV_OK;
}

MV_STATUS mvSFlashInit (MV_SFLASH_INFO * pFlinfo)
{
    MV_STATUS ret;
    MV_U8 manf;
    MV_U16 dev;
    MV_U32 indx;
    MV_BOOL detectFlag = MV_FALSE;

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if ((ret = mvSpiInit(MV_SFLASH_BASIC_SPI_FREQ)) != MV_OK)
    {
        mvOsPrintf("%s ERROR: Failed to initialize the SPI interface!\n", __FUNCTION__);
        return ret;
    }

    if ((ret = mvSFlashIdGet(pFlinfo, &manf, &dev)) != MV_OK)
    {
        mvOsPrintf("%s ERROR: Failed to get the SFlash ID!\n", __FUNCTION__);
        return ret;
    }

    for (indx=0; indx<MV_ARRAY_SIZE(sflash); indx++)
    {
        if ((manf == sflash[indx].manufacturerId) && (dev == sflash[indx].deviceId))
        {
            pFlinfo->manufacturerId = manf;
            pFlinfo->deviceId = dev;
            pFlinfo->index = indx;
            detectFlag = MV_TRUE;
        }
    }

    if(!detectFlag)
    {
        mvOsPrintf("%s ERROR: Unknown SPI flash device!\n", __FUNCTION__);
        return MV_FAIL;
    }

    pFlinfo->sectorSize = sflash[pFlinfo->index].sectorSize;
    pFlinfo->sectorNumber = sflash[pFlinfo->index].sectorNumber;
    pFlinfo->pageSize = sflash[pFlinfo->index].pageSize;

    if ((ret = mvSpiBaudRateSet(sflash[pFlinfo->index].spiMaxFreq)) != MV_OK)
    {
        mvOsPrintf("%s ERROR: Failed to set the SPI frequency!\n", __FUNCTION__);
        return ret;
    }

    if ((ret = mvSFlashStatRegLock(pFlinfo, MV_TRUE)) != MV_OK)
        return ret;

	return MV_OK;
}

MV_STATUS mvSFlashSectorErase (MV_SFLASH_INFO * pFlinfo, MV_U32 secNumber)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_SE_CMND_LENGTH];

    MV_U32 secAddr = (secNumber * pFlinfo->sectorSize);
#if 0
    MV_U32 i;
    MV_U32 * pW = (MV_U32*) (secAddr + pFlinfo->baseAddr);
    MV_U32 erasedWord = 0xFFFFFFFF;
    MV_U32 wordsPerSector = (pFlinfo->sectorSize / sizeof(MV_U32));
    MV_BOOL eraseNeeded = MV_FALSE;
#endif
     
    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if (secNumber >= pFlinfo->sectorNumber)
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter sector number!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }
    
#if 0
     
    for (i=0; i<wordsPerSector; i++)
    {
        if (memcmp(pW, &erasedWord, sizeof(MV_U32)) != 0)
        {
            eraseNeeded = MV_TRUE;
            break;
        }

        ++pW;
    }
    if (!eraseNeeded)
        return MV_OK;
#endif

    cmd[0] = sflash[pFlinfo->index].opcdSE;
	cmd[1] = ((secAddr >> 16) & 0xFF);
	cmd[2] = ((secAddr >> 8) & 0xFF);
	cmd[3] = (secAddr & 0xFF);

	if ((ret = mvWriteEnable(pFlinfo)) != MV_OK)
		return ret;

	if ((ret = mvSpiWriteThenWrite(cmd, MV_SFLASH_SE_CMND_LENGTH, NULL, 0)) != MV_OK)
		return ret;

	if ((ret = mvWaitOnWipClear(pFlinfo)) != MV_OK)
		return ret;

	return MV_OK;
}

MV_STATUS mvSFlashChipErase (MV_SFLASH_INFO * pFlinfo)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_BE_CMND_LENGTH];

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    cmd[0] = sflash[pFlinfo->index].opcdBE;

	if ((ret = mvWriteEnable(pFlinfo)) != MV_OK)
		return ret;

    if ((ret = mvSpiWriteThenWrite(cmd, MV_SFLASH_BE_CMND_LENGTH, NULL, 0)) != MV_OK)
		return ret;

	if ((ret = mvWaitOnChipEraseDone(pFlinfo)) != MV_OK)
		return ret;

	return MV_OK;
}

MV_STATUS mvSFlashBlockRd (MV_SFLASH_INFO * pFlinfo, MV_U32 offset,
						   MV_U8* pReadBuff, MV_U32 buffSize)
{
	MV_U8 cmd[MV_SFLASH_READ_CMND_LENGTH];

    if ((pFlinfo == NULL) || (pReadBuff == NULL))
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    cmd[0] = sflash[pFlinfo->index].opcdREAD;
	cmd[1] = ((offset >> 16) & 0xFF);
	cmd[2] = ((offset >> 8) & 0xFF);
	cmd[3] = (offset & 0xFF);

	return mvSpiWriteThenRead(cmd, MV_SFLASH_READ_CMND_LENGTH, pReadBuff, buffSize, 0);
}

MV_STATUS mvSFlashFastBlockRd (MV_SFLASH_INFO * pFlinfo, MV_U32 offset,
						       MV_U8* pReadBuff, MV_U32 buffSize)
{
    MV_U8 cmd[MV_SFLASH_READ_CMND_LENGTH];
    MV_STATUS ret;

    if ((pFlinfo == NULL) || (pReadBuff == NULL))
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    mvOsPrintf("Setting freq to %d.\n",sflash[pFlinfo->index].spiMaxFastFreq);
    if ((ret = mvSpiBaudRateSet(sflash[pFlinfo->index].spiMaxFastFreq)) != MV_OK)
    {
        mvOsPrintf("%s ERROR: Failed to set the SPI fast frequency!\n", __FUNCTION__);
        return ret;
    }

    cmd[0] = sflash[pFlinfo->index].opcdFSTRD;
    cmd[1] = ((offset >> 16) & 0xFF);
    cmd[2] = ((offset >> 8) & 0xFF);
    cmd[3] = (offset & 0xFF);

    ret = mvSpiWriteThenRead(cmd, MV_SFLASH_READ_CMND_LENGTH, pReadBuff, buffSize,
                             sflash[pFlinfo->index].spiFastRdDummyBytes);

    if ((ret = mvSpiBaudRateSet(sflash[pFlinfo->index].spiMaxFreq)) != MV_OK)
    {
        mvOsPrintf("%s ERROR: Failed to set the SPI frequency!\n", __FUNCTION__);
        return ret;
    }

    return ret;
}

MV_STATUS mvSFlashBlockWr (MV_SFLASH_INFO * pFlinfo, MV_U32 offset,
						   MV_U8* pWriteBuff, MV_U32 buffSize)
{
    MV_STATUS ret;
	MV_U32 data2write	= buffSize;
    MV_U32 preAllOffset = (offset & MV_SFLASH_PAGE_ALLIGN_MASK(MV_M25P_PAGE_SIZE));
    MV_U32 preAllSz		= (preAllOffset ? (MV_M25P_PAGE_SIZE - preAllOffset) : 0);
	MV_U32 writeOffset	= offset;

#ifndef CONFIG_MARVELL
    if(NULL == pWriteBuff)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }
#endif

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if ((offset + buffSize) > mvSFlashSizeGet(pFlinfo))
    {
        DB(mvOsPrintf("%s WARNING: Write exceeds flash size!\n", __FUNCTION__);)
	    return MV_OUT_OF_RANGE;
    }

	if (data2write < preAllSz)
		preAllSz = data2write;

	if (preAllSz)
	{
		if ((ret = mvSFlashPageWr(pFlinfo, writeOffset, pWriteBuff, preAllSz)) != MV_OK)
			return ret;

		writeOffset += preAllSz;
		data2write -= preAllSz;
		pWriteBuff += preAllSz;
	}

	while (data2write >= sflash[pFlinfo->index].pageSize)
	{
		if ((ret = mvSFlashPageWr(pFlinfo, writeOffset, pWriteBuff, sflash[pFlinfo->index].pageSize)) != MV_OK)
			return ret;

		writeOffset += sflash[pFlinfo->index].pageSize;
		data2write -= sflash[pFlinfo->index].pageSize;
		pWriteBuff += sflash[pFlinfo->index].pageSize;
	}

	if (data2write)
	{
		if ((ret = mvSFlashPageWr(pFlinfo, writeOffset, pWriteBuff, data2write)) != MV_OK)
			return ret;
	}

	return MV_OK;
}

MV_STATUS mvSFlashIdGet (MV_SFLASH_INFO * pFlinfo, MV_U8* pManId, MV_U16* pDevId)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_RDID_CMND_LENGTH];
	MV_U8 id[MV_SFLASH_RDID_REPLY_LENGTH];

    if ((pFlinfo == NULL) || (pManId == NULL) || (pDevId == NULL))
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
        return mvSFlashWithDefaultsIdGet(pFlinfo, pManId, pDevId);
    else
        cmd[0] = sflash[pFlinfo->index].opcdRDID;

	if ((ret = mvSpiWriteThenRead(cmd, MV_SFLASH_RDID_CMND_LENGTH, id, MV_SFLASH_RDID_REPLY_LENGTH, 0)) != MV_OK)
		return ret;

	*pManId = id[0];
	*pDevId = 0;
	*pDevId |= (id[1] << 8);
	*pDevId |= id[2];

	return MV_OK;
}

MV_STATUS mvSFlashWpRegionSet (MV_SFLASH_INFO * pFlinfo, MV_SFLASH_WP_REGION wpRegion)
{
    MV_U8 wpMask;

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if (pFlinfo->manufacturerId == MV_M25PXXX_ST_MANF_ID)
    {
        switch (wpRegion)
        {
            case MV_WP_NONE:
                wpMask = MV_M25P_STATUS_BP_NONE;
                break;

            case MV_WP_UPR_1OF128:
                DB(mvOsPrintf("%s WARNING: Invaild option for this flash chip!\n", __FUNCTION__);)
                return MV_NOT_SUPPORTED;

            case MV_WP_UPR_1OF64:
                wpMask = MV_M25P_STATUS_BP_1_OF_64;
                break;

            case MV_WP_UPR_1OF32:
                wpMask = MV_M25P_STATUS_BP_1_OF_32;
                break;

            case MV_WP_UPR_1OF16:
                wpMask = MV_M25P_STATUS_BP_1_OF_16;
                break;

            case MV_WP_UPR_1OF8:
                wpMask = MV_M25P_STATUS_BP_1_OF_8;
                break;

            case MV_WP_UPR_1OF4:
                wpMask = MV_M25P_STATUS_BP_1_OF_4;
                break;

            case MV_WP_UPR_1OF2:
                wpMask = MV_M25P_STATUS_BP_1_OF_2;
                break;

            case MV_WP_ALL:
                wpMask = MV_M25P_STATUS_BP_ALL;
                break;

            default:
                DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __FUNCTION__);)
                return MV_BAD_PARAM;
        }
    }
     
    else if (pFlinfo->manufacturerId == MV_MXIC_MANF_ID)
    {
        switch (wpRegion)
        {
            case MV_WP_NONE:
                wpMask = MV_MX25L_STATUS_BP_NONE;
                break;

            case MV_WP_UPR_1OF128:
                wpMask = MV_MX25L_STATUS_BP_1_OF_128;
                break;

            case MV_WP_UPR_1OF64:
                wpMask = MV_MX25L_STATUS_BP_1_OF_64;
                break;

            case MV_WP_UPR_1OF32:
                wpMask = MV_MX25L_STATUS_BP_1_OF_32;
                break;

            case MV_WP_UPR_1OF16:
                wpMask = MV_MX25L_STATUS_BP_1_OF_16;
                break;

            case MV_WP_UPR_1OF8:
                wpMask = MV_MX25L_STATUS_BP_1_OF_8;
                break;

            case MV_WP_UPR_1OF4:
                wpMask = MV_MX25L_STATUS_BP_1_OF_4;
                break;

            case MV_WP_UPR_1OF2:
                wpMask = MV_MX25L_STATUS_BP_1_OF_2;
                break;

            case MV_WP_ALL:
                wpMask = MV_MX25L_STATUS_BP_ALL;
                break;

            default:
                DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __FUNCTION__);)
                return MV_BAD_PARAM;
        }
    }
     
    else if (pFlinfo->manufacturerId == MV_SPANSION_MANF_ID)
    {
        switch (wpRegion)
        {
            case MV_WP_NONE:
                wpMask = MV_S25FL_STATUS_BP_NONE;
                break;

            case MV_WP_UPR_1OF128:
                DB(mvOsPrintf("%s WARNING: Invaild option for this flash chip!\n", __FUNCTION__);)
                return MV_NOT_SUPPORTED;

            case MV_WP_UPR_1OF64:
                wpMask = MV_S25FL_STATUS_BP_1_OF_64;
                break;

            case MV_WP_UPR_1OF32:
                wpMask = MV_S25FL_STATUS_BP_1_OF_32;
                break;

            case MV_WP_UPR_1OF16:
                wpMask = MV_S25FL_STATUS_BP_1_OF_16;
                break;

            case MV_WP_UPR_1OF8:
                wpMask = MV_S25FL_STATUS_BP_1_OF_8;
                break;

            case MV_WP_UPR_1OF4:
                wpMask = MV_S25FL_STATUS_BP_1_OF_4;
                break;

            case MV_WP_UPR_1OF2:
                wpMask = MV_S25FL_STATUS_BP_1_OF_2;
                break;

            case MV_WP_ALL:
                wpMask = MV_S25FL_STATUS_BP_ALL;
                break;

            default:
                DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __FUNCTION__);)
                return MV_BAD_PARAM;
        }
    }
#ifdef MY_ABC_HERE
     
    else if (pFlinfo->manufacturerId == MV_SST_MANF_ID)
    {
        switch (wpRegion)
        {
            case MV_WP_NONE:
                wpMask = MV_SST25VF_STATUS_BP_NONE;
                break;

            case MV_WP_UPR_1OF64:
                wpMask = MV_SST25VF_STATUS_BP_1_OF_64;
                break;

            case MV_WP_UPR_1OF32:
                wpMask = MV_SST25VF_STATUS_BP_1_OF_32;
                break;

            case MV_WP_UPR_1OF16:
                wpMask = MV_SST25VF_STATUS_BP_1_OF_16;
                break;

            case MV_WP_UPR_1OF8:
                wpMask = MV_SST25VF_STATUS_BP_1_OF_8;
                break;

            case MV_WP_UPR_1OF4:
                wpMask = MV_SST25VF_STATUS_BP_1_OF_4;
                break;

            case MV_WP_UPR_1OF2:
                wpMask = MV_SST25VF_STATUS_BP_1_OF_2;
                break;

            case MV_WP_ALL:
                wpMask = MV_SST25VF_STATUS_BP_ALL;
                break;

        default:
                DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __FUNCTION__);)
                return MV_BAD_PARAM;
        }
    }
#endif
    else
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter Manufacturer ID!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    wpMask |= MV_SFLASH_STATUS_REG_SRWD_MASK;

	return mvStatusRegSet(pFlinfo, wpMask);
}

MV_STATUS mvSFlashWpRegionGet (MV_SFLASH_INFO * pFlinfo, MV_SFLASH_WP_REGION * pWpRegion)
{
    MV_STATUS ret;
	MV_U8 reg;

    if ((pFlinfo == NULL) || (pWpRegion == NULL))
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if ((ret = mvStatusRegGet(pFlinfo, &reg)) != MV_OK)
        return ret;

    if (pFlinfo->manufacturerId == MV_M25PXXX_ST_MANF_ID)
    {
        switch ((reg & MV_M25P_STATUS_REG_WP_MASK))
        {
            case MV_M25P_STATUS_BP_NONE:
                *pWpRegion = MV_WP_NONE;
                break;

            case MV_M25P_STATUS_BP_1_OF_64:
                *pWpRegion = MV_WP_UPR_1OF64;
                break;

            case MV_M25P_STATUS_BP_1_OF_32:
                *pWpRegion = MV_WP_UPR_1OF32;
                break;

            case MV_M25P_STATUS_BP_1_OF_16:
                *pWpRegion = MV_WP_UPR_1OF16;
                break;

            case MV_M25P_STATUS_BP_1_OF_8:
                *pWpRegion = MV_WP_UPR_1OF8;
                break;

            case MV_M25P_STATUS_BP_1_OF_4:
                *pWpRegion = MV_WP_UPR_1OF4;
                break;

            case MV_M25P_STATUS_BP_1_OF_2:
                *pWpRegion = MV_WP_UPR_1OF2;
                break;

            case MV_M25P_STATUS_BP_ALL:
                *pWpRegion = MV_WP_ALL;
                break;

            default:
                DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __FUNCTION__);)
                return MV_BAD_VALUE;
        }
    }
     
    else if (pFlinfo->manufacturerId == MV_MXIC_MANF_ID)
    {
        switch ((reg & MV_MX25L_STATUS_REG_WP_MASK))
        {
            case MV_MX25L_STATUS_BP_NONE:
                *pWpRegion = MV_WP_NONE;
                break;

            case MV_MX25L_STATUS_BP_1_OF_128:
                *pWpRegion = MV_WP_UPR_1OF128;
                break;

            case MV_MX25L_STATUS_BP_1_OF_64:
                *pWpRegion = MV_WP_UPR_1OF64;
                break;

            case MV_MX25L_STATUS_BP_1_OF_32:
                *pWpRegion = MV_WP_UPR_1OF32;
                break;

            case MV_MX25L_STATUS_BP_1_OF_16:
                *pWpRegion = MV_WP_UPR_1OF16;
                break;

            case MV_MX25L_STATUS_BP_1_OF_8:
                *pWpRegion = MV_WP_UPR_1OF8;
                break;

            case MV_MX25L_STATUS_BP_1_OF_4:
                *pWpRegion = MV_WP_UPR_1OF4;
                break;

            case MV_MX25L_STATUS_BP_1_OF_2:
                *pWpRegion = MV_WP_UPR_1OF2;
                break;

            case MV_MX25L_STATUS_BP_ALL:
                *pWpRegion = MV_WP_ALL;
                break;

            default:
                DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __FUNCTION__);)
                return MV_BAD_VALUE;
        }
    }
     
    else if (pFlinfo->manufacturerId == MV_SPANSION_MANF_ID)
    {
        switch ((reg & MV_S25FL_STATUS_REG_WP_MASK))
        {
            case MV_S25FL_STATUS_BP_NONE:
                *pWpRegion = MV_WP_NONE;
                break;

            case MV_S25FL_STATUS_BP_1_OF_64:
                *pWpRegion = MV_WP_UPR_1OF64;
                break;

            case MV_S25FL_STATUS_BP_1_OF_32:
                *pWpRegion = MV_WP_UPR_1OF32;
                break;

            case MV_S25FL_STATUS_BP_1_OF_16:
                *pWpRegion = MV_WP_UPR_1OF16;
                break;

            case MV_S25FL_STATUS_BP_1_OF_8:
                *pWpRegion = MV_WP_UPR_1OF8;
                break;

            case MV_S25FL_STATUS_BP_1_OF_4:
                *pWpRegion = MV_WP_UPR_1OF4;
                break;

            case MV_S25FL_STATUS_BP_1_OF_2:
                *pWpRegion = MV_WP_UPR_1OF2;
                break;

            case MV_S25FL_STATUS_BP_ALL:
                *pWpRegion = MV_WP_ALL;
                break;

            default:
                DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __FUNCTION__);)
                return MV_BAD_VALUE;
        }
    }
#ifdef MY_ABC_HERE
     
    else if (pFlinfo->manufacturerId == MV_SST_MANF_ID)
    {
       switch ((reg & MV_SST25VF_STATUS_REG_WP_MASK))
       {
            case MV_SST25VF_STATUS_BP_NONE:
                *pWpRegion = MV_WP_NONE;
                break;

            case MV_SST25VF_STATUS_BP_1_OF_64:
                *pWpRegion = MV_WP_UPR_1OF64;
                break;

            case MV_SST25VF_STATUS_BP_1_OF_32:
                *pWpRegion = MV_WP_UPR_1OF32;
                break;

            case MV_SST25VF_STATUS_BP_1_OF_16:
                *pWpRegion = MV_WP_UPR_1OF16;
                break;

            case MV_SST25VF_STATUS_BP_1_OF_8:
                *pWpRegion = MV_WP_UPR_1OF8;
                break;

            case MV_SST25VF_STATUS_BP_1_OF_4:
                *pWpRegion = MV_WP_UPR_1OF4;
                break;

            case MV_SST25VF_STATUS_BP_1_OF_2:
                *pWpRegion = MV_WP_UPR_1OF2;
                break;

            case MV_SST25VF_STATUS_BP_ALL:
                *pWpRegion = MV_WP_ALL;
                break;

            default:
                DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __FUNCTION__);)
                return MV_BAD_VALUE;
       }
    }
#endif
    else
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter Manufacturer ID!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

	return MV_OK;
}

MV_STATUS mvSFlashStatRegLock (MV_SFLASH_INFO * pFlinfo, MV_BOOL srLock)
{
    MV_STATUS ret;
	MV_U8 reg;

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if ((ret = mvStatusRegGet(pFlinfo, &reg)) != MV_OK)
        return ret;

	if (srLock)
		reg |= MV_SFLASH_STATUS_REG_SRWD_MASK;
	else
		reg &= ~MV_SFLASH_STATUS_REG_SRWD_MASK;

	return mvStatusRegSet(pFlinfo, reg);
}

MV_U32 mvSFlashSizeGet (MV_SFLASH_INFO * pFlinfo)
{
     
    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return 0;
    }

    return (pFlinfo->sectorSize * pFlinfo->sectorNumber);
}

MV_STATUS mvSFlashPowerSaveEnter(MV_SFLASH_INFO * pFlinfo)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_DP_CMND_LENGTH];

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return 0;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if (sflash[pFlinfo->index].opcdPwrSave == MV_SFLASH_NO_SPECIFIC_OPCD)
    {
        DB(mvOsPrintf("%s WARNING: Power save not supported for this device!\n", __FUNCTION__);)
        return MV_NOT_SUPPORTED;
    }

    cmd[0] = sflash[pFlinfo->index].opcdPwrSave;

    if ((ret = mvSpiWriteThenWrite(cmd, MV_SFLASH_DP_CMND_LENGTH, NULL, 0)) != MV_OK)
		return ret;

	return MV_OK;

}

MV_STATUS mvSFlashPowerSaveExit (MV_SFLASH_INFO * pFlinfo)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_RES_CMND_LENGTH];

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return 0;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return MV_BAD_PARAM;
    }

    if (sflash[pFlinfo->index].opcdRES == MV_SFLASH_NO_SPECIFIC_OPCD)
    {
        DB(mvOsPrintf("%s WARNING: Read Electronic Signature not supported for this device!\n", __FUNCTION__);)
        return MV_NOT_SUPPORTED;
    }

    cmd[0] = sflash[pFlinfo->index].opcdRES;

    if ((ret = mvSpiWriteThenWrite(cmd, MV_SFLASH_RES_CMND_LENGTH, NULL, 0)) != MV_OK)
		return ret;

    mvOsDelay(MV_MXIC_DP_EXIT_DELAY);    

	return MV_OK;

}

const MV_8 * mvSFlashModelGet (MV_SFLASH_INFO * pFlinfo)
{
    static const MV_8 * unknModel = (const MV_8 *)"Unknown";

    if (pFlinfo == NULL)
    {
        mvOsPrintf("%s ERROR: Null pointer parameter!\n", __FUNCTION__);
        return 0;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
    {
        DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __FUNCTION__);)
        return unknModel;
    }

    return sflash[pFlinfo->index].deviceModel;
}
