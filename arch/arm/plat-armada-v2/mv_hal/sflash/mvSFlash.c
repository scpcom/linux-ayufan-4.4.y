 
#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSFlash.h"
#include "mvSFlashSpec.h"
#include "mvSysSFlash.h"

#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

static MV_SFLASH_DEVICE_PARAMS sflash[] = {
#ifdef CONFIG_SYNO_ARMADA_V2
     
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
     "NUMONYX M25P64",
     MV_M25PXXX_ST_MANF_ID,
     MV_NU25P64_DEVICE_ID,
     MV_M25P64_MAX_SPI_FREQ,
     MV_M25P64_MAX_FAST_SPI_FREQ,
     MV_M25P64_FAST_READ_DUMMY_BYTES,
     MV_M25P64_ADDR_CYC_CNT
    }
     
    ,{
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
     MV_MX25L8006E_SECTOR_SIZE,
     MV_MX25L8006E_SECTOR_NUMBER,
     MV_MXIC_PAGE_SIZE,
     "MXIC MX25L8006E",
     MV_MXIC_MANF_ID,
     MV_MX25L8006E_DEVICE_ID,
     MV_MX25L8006E_MAX_SPI_FREQ,
     MV_MX25L8006E_MAX_FAST_SPI_FREQ,
     MV_MX25L8006E_FAST_READ_DUMMY_BYTES,
     MV_MX25L8006E_ADDR_CYC_CNT
    }
    
    ,{
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
     MV_M25P80_SECTOR_SIZE,
     MV_M25P80_SECTOR_NUMBER,
     MV_M25P_PAGE_SIZE,
     "Mircon M25P80",
     MV_M25PXXX_ST_MANF_ID,
     MV_M25P80_DEVICE_ID,
     MV_M25P80_MAX_SPI_FREQ,
     MV_M25P80_MAX_FAST_SPI_FREQ,
     MV_M25P80_FAST_READ_DUMMY_BYTES
    }
    
    ,{
     GD_GD25Q_WREN_CMND_OPCD,
     GD_GD25Q_WRDI_CMND_OPCD,
     GD_GD25Q_RDID_CMND_OPCD,
     GD_GD25Q_RDSR_CMND_OPCD,
     GD_GD25Q_WRSR_CMND_OPCD,
     GD_GD25Q_READ_CMND_OPCD,
     GD_GD25Q_FAST_RD_CMND_OPCD,
     GD_GD25Q_PP_CMND_OPCD,
     GD_GD25Q_SE_CMND_OPCD,
     GD_GD25Q_BE_CMND_OPCD,
     GD_GD25Q_RES_CMND_OPCD,
     GD_SFLASH_NO_SPECIFIC_OPCD,          
     GD_GD25Q_SECTOR_SIZE,
     GD_GD25Q_SECTOR_NUMBER,
     GD_GD25Q_PAGE_SIZE,
     "Gigadevice GD25Q64B",
     GD_GD25Q_MANF_ID,
     GD_GD25Q64B_DEVICE_ID,
     GD_GD25Q_MAX_SPI_FREQ,
     GD_GD25Q_MAX_FAST_SPI_FREQ,
     GD_GD25Q_FAST_READ_DUMMY_BYTES,
	 GD_GD25Q_ADDR_CYC_CNT
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
     MV_M25P32_FAST_READ_DUMMY_BYTES,
     MV_M25P32_ADDR_CYC_CNT
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
     MV_M25P64_FAST_READ_DUMMY_BYTES,
     MV_M25P64_ADDR_CYC_CNT
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
     "ST M25PX64",
     MV_M25PXXX_ST_MANF_ID,
     MV_M25PX64_DEVICE_ID,
     MV_M25PX64_MAX_SPI_FREQ,
     MV_M25PX64_MAX_FAST_SPI_FREQ,
     MV_M25PX64_FAST_READ_DUMMY_BYTES,
     MV_M25PX64_ADDR_CYC_CNT
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
     MV_M25P128_FAST_READ_DUMMY_BYTES,
     MV_M25P128_ADDR_CYC_CNT
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
	MV_M25Q128_SECTOR_SIZE,
	MV_M25Q128_SECTOR_NUMBER,
	MV_M25Q_PAGE_SIZE,
	"ST M25Q128",
	MV_M25PXXX_ST_MANF_ID,
	MV_M25Q128_DEVICE_ID,
	MV_M25Q128_MAX_SPI_FREQ,
	MV_M25Q128_MAX_FAST_SPI_FREQ,
	MV_M25Q128_FAST_READ_DUMMY_BYTES,
	MV_M25Q128_ADDR_CYC_CNT
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
     MV_MX25L1605_SECTOR_SIZE,
     MV_MX25L1605_SECTOR_NUMBER,
     MV_MXIC_PAGE_SIZE,
     "MXIC MX25L1605",
     MV_MXIC_MANF_ID,
     MV_MX25L1605_DEVICE_ID,
     MV_MX25L1605_MAX_SPI_FREQ,
     MV_MX25L1605_MAX_FAST_SPI_FREQ,
     MV_MX25L1605_FAST_READ_DUMMY_BYTES,
     MV_MX25L1605_ADDR_CYC_CNT
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
     MV_MX25L3205_SECTOR_SIZE,
     MV_MX25L3205_SECTOR_NUMBER,
     MV_MXIC_PAGE_SIZE,
     "MXIC MX25L3205",
     MV_MXIC_MANF_ID,
     MV_MX25L3205_DEVICE_ID,
     MV_MX25L3205_MAX_SPI_FREQ,
     MV_MX25L3205_MAX_FAST_SPI_FREQ,
     MV_MX25L3205_FAST_READ_DUMMY_BYTES,
     MV_MX25L3205_ADDR_CYC_CNT
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
     MV_MX25L6405_SECTOR_SIZE,
     MV_MX25L6405_SECTOR_NUMBER,
     MV_MXIC_PAGE_SIZE,
     "MXIC MX25L6405",
     MV_MXIC_MANF_ID,
     MV_MX25L6405_DEVICE_ID,
     MV_MX25L6405_MAX_SPI_FREQ,
     MV_MX25L6405_MAX_FAST_SPI_FREQ,
     MV_MX25L6405_FAST_READ_DUMMY_BYTES,
     MV_MX25L6405_ADDR_CYC_CNT
    },
	 
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
	    MV_S25FL032_SECTOR_SIZE,
	    MV_S25FL032_SECTOR_NUMBER,
	    MV_S25FL_PAGE_SIZE,
	    "SPANSION S25FL032",
	    MV_SPANSION_MANF_ID,
	    MV_S25FL032_DEVICE_ID,
	    MV_S25FL032_MAX_SPI_FREQ,
 
	},
     
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
     MV_M25P128_FAST_READ_DUMMY_BYTES,
     MV_M25P128_ADDR_CYC_CNT
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
	    MV_MX25L257_SECTOR_SIZE,
	    MV_MX25L257_SECTOR_NUMBER,
	    MV_MXIC_PAGE_SIZE,
	    "MACRONIX MX25L25735E",
	    MV_MXIC_MANF_ID,
	    MV_MX25L257_DEVICE_ID,
	    MV_MX25L257_MAX_SPI_FREQ,
	    MV_MX25L257_MAX_FAST_SPI_FREQ,
	    MV_MX25L257_FAST_READ_DUMMY_BYTES,
	    MV_MX25L257_ADDR_CYC_CNT
	},
	 
	{
	    MV_WB25Q_WREN_CMND_OPCD,
	    MV_WB25Q_WRDI_CMND_OPCD,
	    MV_WB25Q_RDID_CMND_OPCD,
	    MV_WB25Q_RDSR_CMND_OPCD,
	    MV_WB25Q_WRSR_CMND_OPCD,
	    MV_WB25Q_READ_CMND_OPCD,
	    MV_WB25Q_FAST_RD_CMND_OPCD,
	    MV_WB25Q_PP_CMND_OPCD,
	    MV_WB25Q_SE_CMND_OPCD,
	    MV_WB25Q_BE_CMND_OPCD,
	    MV_WB25Q_RES_CMND_OPCD,
	    MV_WB25Q_DP_CMND_OPCD,
	    MV_WB25Q32_SECTOR_SIZE,
	    MV_WB25Q32_SECTOR_NUMBER,
	    MV_WB25Q_PAGE_SIZE,
	    "WINBOND WB25Q32",
	    MV_WINBOND_MANF_ID,
	    MV_WB25Q32_DEVICE_ID,
	    MV_WB25Q32_MAX_SPI_FREQ,
	    MV_WB25Q32_MAX_FAST_SPI_FREQ,
	    MV_WB25Q32_FAST_READ_DUMMY_BYTES,
	    MV_WB25Q32_ADDR_CYC_CNT
	}
};

static MV_STATUS    mvWriteEnable(MV_SFLASH_INFO *pFlinfo);
static MV_STATUS    mvStatusRegGet(MV_SFLASH_INFO *pFlinfo, MV_U8 *pStatReg);
static MV_STATUS    mvStatusRegSet(MV_SFLASH_INFO *pFlinfo, MV_U8 sr);
static MV_STATUS    mvWaitOnWipClear(MV_SFLASH_INFO *pFlinfo);
static MV_STATUS    mvSFlashPageWr(MV_SFLASH_INFO *pFlinfo, MV_U32 offset, \
					MV_U8 *pPageBuff, MV_U32 buffSize);
static MV_STATUS    mvSFlashWithDefaultsIdGet(MV_SFLASH_INFO *pFlinfo, \
						MV_U8 *manId, MV_U16 *devId);

static MV_STATUS mvWriteEnable(MV_SFLASH_INFO *pFlinfo)
{
	MV_U8 cmd[MV_SFLASH_WREN_CMND_LENGTH];

	cmd[0] = sflash[pFlinfo->index].opcdWREN;

	return mvSysSflashCommandSet(0, cmd, MV_SFLASH_WREN_CMND_LENGTH,
			SYS_SFLASH_TRANS_ATOMIC);
}

static MV_STATUS mvStatusRegGet(MV_SFLASH_INFO *pFlinfo, MV_U8 *pStatReg)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_RDSR_CMND_LENGTH];
	MV_U8 sr[MV_SFLASH_RDSR_REPLY_LENGTH];

	cmd[0] = sflash[pFlinfo->index].opcdRDSR;

	ret = mvSysSflashCommandSet(NULL,  cmd, MV_SFLASH_RDSR_CMND_LENGTH,
			SYS_SFLASH_TRANS_START);
	if (ret == MV_OK) {
		ret = mvSysSflashDataRead(NULL, sr, MV_SFLASH_RDSR_REPLY_LENGTH, 0,
				SYS_SFLASH_TRANS_END);
	}
	if (ret != MV_OK)
		return ret;

    *pStatReg = sr[0];

    return MV_OK;
}

static MV_STATUS mvWaitOnWipClear(MV_SFLASH_INFO *pFlinfo)
{
    MV_STATUS ret;
	MV_U32 i;
    MV_U8 stat;

	for (i = 0; i < MV_SFLASH_MAX_WAIT_LOOP; i++) {
	ret = mvStatusRegGet(pFlinfo, &stat);
	if (ret != MV_OK)
		return ret;

		if ((stat & MV_SFLASH_STATUS_REG_WIP_MASK) == 0)
			return MV_OK;
	}

    DB(mvOsPrintf("%s WARNING: Write Timeout!\n", __func__);)
	return MV_TIMEOUT;
}

static MV_STATUS mvWaitOnChipEraseDone(MV_SFLASH_INFO *pFlinfo)
{
    MV_STATUS ret;
	MV_U32 i;
    MV_U8 stat;

	for (i = 0; i < MV_SFLASH_CHIP_ERASE_MAX_WAIT_LOOP; i++) {
	ret = mvStatusRegGet(pFlinfo, &stat);
	if (ret != MV_OK)
		return ret;

		if ((stat & MV_SFLASH_STATUS_REG_WIP_MASK) == 0)
			return MV_OK;
	}

    DB(mvOsPrintf("%s WARNING: Write Timeout!\n", __func__);)
	return MV_TIMEOUT;
}

static MV_STATUS mvStatusRegSet(MV_SFLASH_INFO *pFlinfo, MV_U8 sr)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_WRSR_CMND_LENGTH];

	ret = mvWriteEnable(pFlinfo);
	if (ret != MV_OK)
		return ret;

	cmd[0] = sflash[pFlinfo->index].opcdWRSR;
	cmd[1] = sr;

	ret = mvSysSflashCommandSet(NULL,  cmd, MV_SFLASH_WRSR_CMND_LENGTH, SYS_SFLASH_TRANS_ATOMIC);
	if (ret != MV_OK)
		return ret;
	ret = mvWaitOnWipClear(pFlinfo);
	if (ret != MV_OK)
		return ret;

	mvOsDelay(1);
	return MV_OK;
}

static MV_STATUS mvSFlashPageWr(MV_SFLASH_INFO *pFlinfo, MV_U32 offset,
							     MV_U8 *pPageBuff, MV_U32 buffSize)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_PP_CMND_LENGTH];
	MV_U32 i;

	if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
		DB(mvOsPrintf("%s WARNING: Invalid parameter device index!\n", __func__););
		return MV_BAD_PARAM;
	}

	if (((offset & (sflash[pFlinfo->index].pageSize - 1)) + buffSize) >
			sflash[pFlinfo->index].pageSize) {
		DB(mvOsPrintf("%s WARNING: Page allignment problem!\n", __func__););
		return MV_OUT_OF_RANGE;
	}

	ret = mvWriteEnable(pFlinfo);
	if (ret != MV_OK)
		return ret;

	cmd[0] = sflash[pFlinfo->index].opcdPP;
	for (i = 1; i <= sflash[pFlinfo->index].addrCycCnt; i++)
		cmd[i] = (offset >> ((sflash[pFlinfo->index].addrCycCnt - i) * 8)) & 0xFF;

	ret = mvSysSflashCommandSet(NULL, cmd, sflash[pFlinfo->index].addrCycCnt + 1, SYS_SFLASH_TRANS_START);
	if (ret == MV_OK)
		ret = mvSysSflashDataWrite(NULL, pPageBuff, buffSize, SYS_SFLASH_TRANS_END);
	if (ret != MV_OK)
		return ret;
	ret = mvWaitOnWipClear(pFlinfo);
	if (ret != MV_OK)
		return ret;

	return MV_OK;
}

static MV_STATUS mvSFlashWithDefaultsIdGet(MV_SFLASH_INFO *pFlinfo, MV_U8 *manId, MV_U16 *devId)
{
    MV_STATUS ret;
    MV_U8 cmdRDID[MV_SFLASH_RDID_CMND_LENGTH];
	MV_U8 id[MV_SFLASH_RDID_REPLY_LENGTH] = {0};

	cmdRDID[0] = MV_SFLASH_DEFAULT_RDID_OPCD;    

	ret = mvSysSflashCommandSet(NULL,  cmdRDID, MV_SFLASH_RDID_CMND_LENGTH, SYS_SFLASH_TRANS_START);
	if (ret == MV_OK)
		ret = mvSysSflashDataRead(NULL, id, MV_SFLASH_RDID_REPLY_LENGTH, 0, SYS_SFLASH_TRANS_END);

	*manId = id[0];
	*devId = 0;
	*devId |= (id[1] << 8);
	*devId |= id[2];

	return MV_OK;
}

MV_STATUS mvSFlashInit(MV_SFLASH_INFO *pFlinfo)
{
    MV_STATUS ret;
    MV_U8 manf;
    MV_U16 dev;
    MV_U32 indx;
    MV_BOOL detectFlag = MV_FALSE;

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    ret = mvSysSflashFreqSet(NULL, MV_SFLASH_BASIC_SPI_FREQ);
    if (ret != MV_OK) {
	mvOsPrintf("%s ERROR: Failed to set base SPI frequency!\n", __func__);
	return ret;
    }

    ret = mvSFlashIdGet(pFlinfo, &manf, &dev);
    if (ret != MV_OK) {
	mvOsPrintf("%s ERROR: Failed to get the SFlash ID!\n", __func__);
	return ret;
    }

    for (indx = 0; indx < MV_ARRAY_SIZE(sflash); indx++) {
	if ((manf == sflash[indx].manufacturerId) && (dev == sflash[indx].deviceId)) {
		pFlinfo->manufacturerId = manf;
		pFlinfo->deviceId = dev;
		pFlinfo->index = indx;
		detectFlag = MV_TRUE;
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
		break;
#endif
	}
    }

    if (!detectFlag) {
	mvOsPrintf("%s ERROR: Unknown SPI flash device!\n", __func__);
	mvOsPrintf("%s ERROR: Manufacturer = %d Device = %d\n", __func__,
			manf, dev);
	return MV_FAIL;
    }

    pFlinfo->sectorSize = sflash[pFlinfo->index].sectorSize;
    pFlinfo->sectorNumber = sflash[pFlinfo->index].sectorNumber;
    pFlinfo->pageSize = sflash[pFlinfo->index].pageSize;

    ret = mvSysSflashFreqSet(NULL, sflash[pFlinfo->index].spiMaxFreq);
    if (ret != MV_OK) {
	 mvOsPrintf("%s ERROR: Failed to set the SPI frequency!\n", __func__);
	return ret;
    }

    ret = mvSFlashStatRegLock(pFlinfo, MV_TRUE);
    if (ret != MV_OK)
	return ret;

	return MV_OK;
}

MV_STATUS mvSFlashSectorErase(MV_SFLASH_INFO *pFlinfo, MV_U32 secNumber)
{
    MV_STATUS ret;
    MV_U8 cmd[MV_SFLASH_SE_CMND_LENGTH];
    MV_U32 secAddr;
    MV_U32 i;

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }
    secAddr = (secNumber * pFlinfo->sectorSize);

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }

    if (secNumber >= pFlinfo->sectorNumber) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter sector number!\n", __func__);)
	return MV_BAD_PARAM;
    }

    cmd[0] = sflash[pFlinfo->index].opcdSE;
	for (i = 1; i <= sflash[pFlinfo->index].addrCycCnt; i++)
		cmd[i] = (secAddr >> ((sflash[pFlinfo->index].addrCycCnt - i) * 8)) & 0xFF;

	ret = mvWriteEnable(pFlinfo);
	if (ret != MV_OK)
		return ret;

	ret = mvSysSflashCommandSet(NULL, cmd, sflash[pFlinfo->index].addrCycCnt + 1, SYS_SFLASH_TRANS_ATOMIC);
	if (ret != MV_OK)
		return ret;
	ret = mvWaitOnWipClear(pFlinfo);
	if (ret != MV_OK)
		return ret;

	return MV_OK;
}

MV_STATUS mvSFlashChipErase(MV_SFLASH_INFO *pFlinfo)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_BE_CMND_LENGTH];

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }

    cmd[0] = sflash[pFlinfo->index].opcdBE;

	ret = mvWriteEnable(pFlinfo);
	if (ret != MV_OK)
		return ret;

	ret = mvSysSflashCommandSet(NULL, cmd, MV_SFLASH_BE_CMND_LENGTH, SYS_SFLASH_TRANS_ATOMIC);
	if (ret != MV_OK)
		return ret;
	ret = mvWaitOnChipEraseDone(pFlinfo);
	if (ret != MV_OK)
		return ret;

	return MV_OK;
}

MV_STATUS mvSFlashBlockRd(MV_SFLASH_INFO *pFlinfo, MV_U32 offset,
						   MV_U8 *pReadBuff, MV_U32 buffSize)
{
	MV_U8 cmd[MV_SFLASH_READ_CMND_LENGTH];
	MV_STATUS status;
	MV_U32 i;

	if ((pFlinfo == NULL) || (pReadBuff == NULL)) {
		mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
		return MV_BAD_PARAM;
	}

	if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
		DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__));
		return MV_BAD_PARAM;
	}

	cmd[0] = sflash[pFlinfo->index].opcdREAD;
	for (i = 1; i <= sflash[pFlinfo->index].addrCycCnt; i++)
		cmd[i] = (offset >> ((sflash[pFlinfo->index].addrCycCnt - i) * 8)) & 0xFF;

	status = mvSysSflashCommandSet(NULL, cmd, sflash[pFlinfo->index].addrCycCnt + 1,
			SYS_SFLASH_TRANS_START);
	if (status == MV_OK)
		status = mvSysSflashDataRead(NULL, pReadBuff, buffSize, 0,
				SYS_SFLASH_TRANS_END);
	return status;
}

MV_STATUS mvSFlashFastBlockRd(MV_SFLASH_INFO *pFlinfo, MV_U32 offset,
						       MV_U8 *pReadBuff, MV_U32 buffSize)
{
    MV_U8 cmd[MV_SFLASH_READ_CMND_LENGTH];
    MV_STATUS ret, retCmd;
    MV_U32 i;

    if ((pFlinfo == NULL) || (pReadBuff == NULL)) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }

    cmd[0] = sflash[pFlinfo->index].opcdFSTRD;
    for (i = 1; i <= sflash[pFlinfo->index].addrCycCnt; i++)
	cmd[i] = (offset >> ((sflash[pFlinfo->index].addrCycCnt - i) * 8)) & 0xFF;

    retCmd = mvSysSflashCommandSet(NULL, cmd, sflash[pFlinfo->index].addrCycCnt + 1,
		    SYS_SFLASH_TRANS_START);

    mvOsPrintf("Setting freq to %d.\n", sflash[pFlinfo->index].spiMaxFastFreq);

    ret = mvSysSflashFreqSet(NULL, sflash[pFlinfo->index].spiMaxFastFreq);
    if (ret != MV_OK) {
	mvOsPrintf("%s ERROR: Failed to set the SPI fast frequency!\n", __func__);
	return ret;
    }

    if (retCmd == MV_OK)
	ret = mvSysSflashDataRead(NULL, pReadBuff, buffSize,
			    sflash[pFlinfo->index].spiFastRdDummyBytes, SYS_SFLASH_TRANS_END);

    ret = mvSysSflashFreqSet(NULL, sflash[pFlinfo->index].spiMaxFreq);
    if (ret != MV_OK) {
	mvOsPrintf("%s ERROR: Failed to set the SPI frequency!\n", __func__);
	return ret;
    }

    return retCmd;
}

MV_STATUS mvSFlashBlockWr(MV_SFLASH_INFO *pFlinfo, MV_U32 offset,
						   MV_U8 *pWriteBuff, MV_U32 buffSize)
{
    MV_STATUS ret;
	MV_U32 data2write	= buffSize;
    MV_U32 preAllOffset = (offset & MV_SFLASH_PAGE_ALLIGN_MASK(MV_M25P_PAGE_SIZE));
    MV_U32 preAllSz		= (preAllOffset ? (MV_M25P_PAGE_SIZE - preAllOffset) : 0);
	MV_U32 writeOffset	= offset;

#ifndef CONFIG_MARVELL
    if (NULL == pWriteBuff) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }
#endif

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }

    if ((offset + buffSize) > mvSFlashSizeGet(pFlinfo)) {
	DB(mvOsPrintf("%s WARNING: Write exceeds flash size!\n", __func__);)
	return MV_OUT_OF_RANGE;
    }

	if (data2write < preAllSz)
		preAllSz = data2write;

	if (preAllSz) {
		ret = mvSFlashPageWr(pFlinfo, writeOffset, pWriteBuff, preAllSz);
		if (ret != MV_OK)
			return ret;

		writeOffset += preAllSz;
		data2write -= preAllSz;
		pWriteBuff += preAllSz;
	}

	while (data2write >= sflash[pFlinfo->index].pageSize) {
		ret = mvSFlashPageWr(pFlinfo, writeOffset, pWriteBuff, sflash[pFlinfo->index].pageSize);
		if (ret != MV_OK)
			return ret;

		writeOffset += sflash[pFlinfo->index].pageSize;
		data2write -= sflash[pFlinfo->index].pageSize;
		pWriteBuff += sflash[pFlinfo->index].pageSize;
	}

	if (data2write) {
		ret = mvSFlashPageWr(pFlinfo, writeOffset, pWriteBuff, data2write);
		if (ret != MV_OK)
			return ret;
	}

	return MV_OK;
}

MV_STATUS mvSFlashIdGet(MV_SFLASH_INFO *pFlinfo, MV_U8 *pManId, MV_U16 *pDevId)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_RDID_CMND_LENGTH];
	MV_U8 id[MV_SFLASH_RDID_REPLY_LENGTH];

    if ((pFlinfo == NULL) || (pManId == NULL) || (pDevId == NULL)) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash))
	return mvSFlashWithDefaultsIdGet(pFlinfo, pManId, pDevId);
    else
	cmd[0] = sflash[pFlinfo->index].opcdRDID;

	ret = mvSysSflashCommandSet(NULL, cmd, MV_SFLASH_RDID_CMND_LENGTH,
			SYS_SFLASH_TRANS_START);
	if (ret == MV_OK)
		ret = mvSysSflashDataRead(NULL, id, MV_SFLASH_RDID_REPLY_LENGTH, 0, SYS_SFLASH_TRANS_END);
	if (ret != MV_OK)
		return ret;

	*pManId = id[0];
	*pDevId = 0;
	*pDevId |= (id[1] << 8);
	*pDevId |= id[2];

	return MV_OK;
}

MV_STATUS mvSFlashWpRegionSet(MV_SFLASH_INFO *pFlinfo, MV_SFLASH_WP_REGION wpRegion)
{
    MV_U8 wpMask;

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }

    if (pFlinfo->manufacturerId == MV_M25PXXX_ST_MANF_ID) {
	switch (wpRegion) {
	case MV_WP_NONE:
	    wpMask = MV_M25P_STATUS_BP_NONE;
	    break;

	case MV_WP_UPR_1OF128:
	    DB(mvOsPrintf("%s WARNING: Invaild option for this flash chip!\n", __func__);)
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
	    DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __func__);)
	    return MV_BAD_PARAM;
	}
    } else if (pFlinfo->manufacturerId == MV_MXIC_MANF_ID) {
	 
	switch (wpRegion) {
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
	    DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __func__);)
	    return MV_BAD_PARAM;
	}
    } else if (pFlinfo->manufacturerId == MV_SPANSION_MANF_ID) {
	 
	switch (wpRegion) {
	case MV_WP_NONE:
	    wpMask = MV_S25FL_STATUS_BP_NONE;
	    break;

	case MV_WP_UPR_1OF128:
	    DB(mvOsPrintf("%s WARNING: Invaild option for this flash chip!\n", __func__);)
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
	    DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __func__);)
	    return MV_BAD_PARAM;
	}
#ifdef CONFIG_SYNO_ARMADA_V2
    } else if (pFlinfo->manufacturerId == GD_GD25Q_MANF_ID) {
		switch (wpRegion) {
			case MV_WP_NONE:
				wpMask = GD_GD25Q_STATUS_BP_NONE;
				break;
			case MV_WP_UPR_1OF256 :
				wpMask = GD_GD25Q_STATUS_BP_1_OF_256;
				break;
			case MV_WP_UPR_1OF128:
				wpMask = GD_GD25Q_STATUS_BP_1_OF_128;
				break;
			case MV_WP_UPR_1OF64:
				wpMask = GD_GD25Q_STATUS_BP_1_OF_64;
				break;
			case MV_WP_UPR_1OF32:
				wpMask = GD_GD25Q_STATUS_BP_1_OF_32;
				break;
			case MV_WP_UPR_1OF16:
				wpMask = GD_GD25Q_STATUS_BP_1_OF_16;
				break;
			case MV_WP_UPR_1OF8:
				wpMask = GD_GD25Q_STATUS_BP_1_OF_8;
				break;
			case MV_WP_UPR_1OF4:
				wpMask = GD_GD25Q_STATUS_BP_1_OF_4;
				break;
			case MV_WP_UPR_1OF2:
				wpMask = GD_GD25Q_STATUS_BP_1_OF_2;
				break;
			case MV_WP_ALL:
				wpMask = GD_GD25Q_STATUS_BP_ALL;
				break;
			default:
				DB(mvOsPrintf("%s WARNING: Invaild parameter WP region!\n", __func__);)
			    return MV_BAD_PARAM;
		}
#endif  
    } else {
	DB(mvOsPrintf("%s WARNING: Invaild parameter Manufacturer ID!\n", __func__);)
	return MV_BAD_PARAM;
    }

    wpMask |= MV_SFLASH_STATUS_REG_SRWD_MASK;
#ifdef CONFIG_SYNO_ARMADA_V2
	 
	if (pFlinfo->manufacturerId == GD_GD25Q_MANF_ID)
		wpMask |= GD_SFLASH_STATUS_REG_WEL_MASK;
#endif

	return mvStatusRegSet(pFlinfo, wpMask);
}

MV_STATUS mvSFlashWpRegionGet(MV_SFLASH_INFO *pFlinfo, MV_SFLASH_WP_REGION *pWpRegion)
{
	MV_STATUS ret;
	MV_U8 reg;

    if ((pFlinfo == NULL) || (pWpRegion == NULL)) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }
    ret = mvStatusRegGet(pFlinfo, &reg);
    if (ret != MV_OK)
	return ret;

    if (pFlinfo->manufacturerId == MV_M25PXXX_ST_MANF_ID) {
	switch ((reg & MV_M25P_STATUS_REG_WP_MASK)) {
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
	    DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __func__);)
	    return MV_BAD_VALUE;
	}
    } else if (pFlinfo->manufacturerId == MV_MXIC_MANF_ID) {
	 
	switch ((reg & MV_MX25L_STATUS_REG_WP_MASK))  {
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
	    DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __func__);)
	    return MV_BAD_VALUE;
	}
    } else if (pFlinfo->manufacturerId == MV_SPANSION_MANF_ID) {
	 
	switch ((reg & MV_S25FL_STATUS_REG_WP_MASK)) {
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
	    DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __func__);)
	    return MV_BAD_VALUE;
	}
#ifdef CONFIG_SYNO_ARMADA_V2
    } else if (pFlinfo->manufacturerId == GD_GD25Q_MANF_ID) {
		switch ((reg & GD_GD25Q_STATUS_REG_WP_MASK)) {
			case MV_WP_NONE :
				*pWpRegion = GD_GD25Q_STATUS_BP_NONE;
				break;
			case MV_WP_UPR_1OF256:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_256;
				break;
			case MV_WP_UPR_1OF128:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_128;
				break;
			case MV_WP_UPR_1OF64:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_64;
				break;
			case MV_WP_UPR_1OF32:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_32;
				break;
			case MV_WP_UPR_1OF16:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_16;
				break;
			case MV_WP_UPR_1OF8:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_8;
				break;
			case MV_WP_UPR_1OF4:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_4;
				break;
			case MV_WP_UPR_1OF2:
				*pWpRegion = GD_GD25Q_STATUS_BP_1_OF_2;
				break;
			case MV_WP_ALL:
				*pWpRegion = GD_GD25Q_STATUS_BP_ALL;
				break;
			default:
				DB(mvOsPrintf("%s WARNING: Unidentified WP region in h/w!\n", __func__);)
					return MV_BAD_VALUE;
		}
#endif   
    } else {
	DB(mvOsPrintf("%s WARNING: Invaild parameter Manufacturer ID!\n", __func__);)
	return MV_BAD_PARAM;
    }

	return MV_OK;
}

MV_STATUS mvSFlashStatRegLock(MV_SFLASH_INFO *pFlinfo, MV_BOOL srLock)
{
    MV_STATUS ret;
	MV_U8 reg;

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return MV_BAD_PARAM;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }
    ret = mvStatusRegGet(pFlinfo, &reg);
    if (ret != MV_OK)
	return ret;

	if (srLock)
		reg |= MV_SFLASH_STATUS_REG_SRWD_MASK;
	else
		reg &= ~MV_SFLASH_STATUS_REG_SRWD_MASK;

	return mvStatusRegSet(pFlinfo, reg);
}

MV_U32 mvSFlashSizeGet(MV_SFLASH_INFO *pFlinfo)
{
     
    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return 0;
    }

    return (pFlinfo->sectorSize * pFlinfo->sectorNumber);
}

MV_STATUS mvSFlashPowerSaveEnter(MV_SFLASH_INFO *pFlinfo)
{
    MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_DP_CMND_LENGTH];

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return 0;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }

    if (sflash[pFlinfo->index].opcdPwrSave == MV_SFLASH_NO_SPECIFIC_OPCD) {
	DB(mvOsPrintf("%s WARNING: Power save not supported for this device!\n", __func__);)
	return MV_NOT_SUPPORTED;
    }

    cmd[0] = sflash[pFlinfo->index].opcdPwrSave;

	ret = mvSysSflashCommandSet(NULL, cmd, MV_SFLASH_DP_CMND_LENGTH, SYS_SFLASH_TRANS_ATOMIC);
	return ret;
}

MV_STATUS mvSFlashPowerSaveExit(MV_SFLASH_INFO *pFlinfo)
{
	MV_STATUS ret;
	MV_U8 cmd[MV_SFLASH_RES_CMND_LENGTH];

    if (pFlinfo == NULL) {
	mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return 0;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return MV_BAD_PARAM;
    }

    if (sflash[pFlinfo->index].opcdRES == MV_SFLASH_NO_SPECIFIC_OPCD) {
	DB(mvOsPrintf("%s WARNING: Read Electronic Signature not supported for this device!\n", __func__);)
	return MV_NOT_SUPPORTED;
    }

    cmd[0] = sflash[pFlinfo->index].opcdRES;

	ret = mvSysSflashCommandSet(NULL, cmd, MV_SFLASH_RES_CMND_LENGTH, SYS_SFLASH_TRANS_ATOMIC);
	if (ret != MV_OK)
		return ret;

    mvOsDelay(MV_MXIC_DP_EXIT_DELAY);    

    return MV_OK;
}

const MV_8 *mvSFlashModelGet(MV_SFLASH_INFO *pFlinfo)
{
    static const MV_8 * unknModel = (const MV_8 *)"Unknown";

    if (pFlinfo == NULL) {
	 mvOsPrintf("%s ERROR: Null pointer parameter!\n", __func__);
	return 0;
    }

    if (pFlinfo->index >= MV_ARRAY_SIZE(sflash)) {
	DB(mvOsPrintf("%s WARNING: Invaild parameter index!\n", __func__);)
	return unknModel;
    }

    return sflash[pFlinfo->index].deviceModel;
}
