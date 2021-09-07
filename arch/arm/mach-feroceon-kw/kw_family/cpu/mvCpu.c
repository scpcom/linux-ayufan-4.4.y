#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "cpu/mvCpu.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"
#include "ctrlEnv/sys/mvCpuIfRegs.h"

#ifdef MV_DEBUG
	#define DB(x)	x
#else
	#define DB(x)
#endif	

static MV_U32 mvCpu6180PclkGet(MV_VOID)
{
	MV_U32 	tmpPClkRate=0;
	MV_CPU_ARM_CLK cpu6180_ddr_l2_CLK[] = MV_CPU6180_DDR_L2_CLCK_TBL;

	tmpPClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	tmpPClkRate = tmpPClkRate & MSAR_CPUCLCK_MASK_6180;
	tmpPClkRate = tmpPClkRate >> MSAR_CPUCLCK_OFFS_6180;
			
	tmpPClkRate = cpu6180_ddr_l2_CLK[tmpPClkRate].cpuClk;

	return tmpPClkRate;
}

MV_U32 mvCpuPclkGet(MV_VOID)
{
#if defined(PCLCK_AUTO_DETECT)
	MV_U32 	tmpPClkRate=0;
	MV_U32 cpuCLK[] = MV_CPU_CLCK_TBL;

	if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
		return mvCpu6180PclkGet();

	tmpPClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	tmpPClkRate = MSAR_CPUCLCK_EXTRACT(tmpPClkRate);
	tmpPClkRate = cpuCLK[tmpPClkRate];

	return tmpPClkRate;
#else
	return MV_DEFAULT_PCLK
#endif
}
#ifdef MY_ABC_HERE
EXPORT_SYMBOL(mvCpuPclkGet);
#endif

static MV_U32  mvCpu6180L2ClkGet(MV_VOID)
{
	MV_U32 	L2ClkRate=0;
	MV_CPU_ARM_CLK _cpu6180_ddr_l2_CLK[] = MV_CPU6180_DDR_L2_CLCK_TBL;

	L2ClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	L2ClkRate = L2ClkRate & MSAR_CPUCLCK_MASK_6180;
	L2ClkRate = L2ClkRate >> MSAR_CPUCLCK_OFFS_6180;
			
	L2ClkRate = _cpu6180_ddr_l2_CLK[L2ClkRate].l2Clk;

	return L2ClkRate;

}

MV_U32  mvCpuL2ClkGet(MV_VOID)
{
#ifdef L2CLK_AUTO_DETECT
	MV_U32 L2ClkRate, tmp, pClkRate, indexL2Rtio;
	MV_U32 L2Rtio[][2] = MV_L2_CLCK_RTIO_TBL;

	if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
		return mvCpu6180L2ClkGet();

	pClkRate = mvCpuPclkGet();

	tmp = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	indexL2Rtio = MSAR_L2CLCK_EXTRACT(tmp);

	L2ClkRate = ((pClkRate * L2Rtio[indexL2Rtio][1]) / L2Rtio[indexL2Rtio][0]);
	return L2ClkRate;
#else
	return MV_BOARD_DEFAULT_L2CLK;
#endif
}

MV_VOID mvCpuNameGet(char *pNameBuff)
{
    MV_U32 cpuModel;
    
    cpuModel = mvOsCpuPartGet();

    switch(cpuModel)
    {
        case CPU_PART_MRVL131:
            mvOsSPrintf(pNameBuff, "%s (Rev %d)", "Marvell Feroceon",mvOsCpuRevGet());
            break;
        case CPU_PART_ARM926:
            mvOsSPrintf(pNameBuff, "%s (Rev %d)", "ARM926",mvOsCpuRevGet());
            break;
	case CPU_PART_ARM946:
		mvOsSPrintf(pNameBuff, "%s (Rev %d)", "ARM946",mvOsCpuRevGet());
		break;
        default:
            mvOsSPrintf(pNameBuff,"??? (0x%04x) (Rev %d)",cpuModel,mvOsCpuRevGet());
            break;
    }   

    return;
}

#define MV_PROC_STR_SIZE 50

static void mvCpuIfGetL2EccMode(MV_8 *buf)
{
    MV_U32 regVal = MV_REG_READ(CPU_L2_CONFIG_REG);
    if (regVal & BIT2)
	mvOsSPrintf(buf, "L2 ECC Enabled");
    else
	mvOsSPrintf(buf, "L2 ECC Disabled");
}

static void mvCpuIfGetL2Mode(MV_8 *buf)
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc	p15, 1, %0, c15, c1, 0" : "=r" (regVal));  
    if (regVal & BIT22)
	mvOsSPrintf(buf, "L2 Enabled");
    else
	mvOsSPrintf(buf, "L2 Disabled");
}

static void mvCpuIfGetL2PrefetchMode(MV_8 *buf)
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc	p15, 1, %0, c15, c1, 0" : "=r" (regVal));  
    if (regVal & BIT24)
	mvOsSPrintf(buf, "L2 Prefetch Disabled");
    else
	mvOsSPrintf(buf, "L2 Prefetch Enabled");
}

static void mvCpuIfGetWriteAllocMode(MV_8 *buf)
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc	p15, 1, %0, c15, c1, 0" : "=r" (regVal));  
    if (regVal & BIT28)
	mvOsSPrintf(buf, "Write Allocate Enabled");
    else
	mvOsSPrintf(buf, "Write Allocate Disabled");
}

static void mvCpuIfGetCpuStreamMode(MV_8 *buf)
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc	p15, 1, %0, c15, c1, 0" : "=r" (regVal));  
    if (regVal & BIT29)
	mvOsSPrintf(buf, "CPU Streaming Enabled");
    else
	mvOsSPrintf(buf, "CPU Streaming Disabled");
}

static void mvCpuIfPrintCpuRegs(void)
{
    MV_U32 regVal = 0;

    __asm volatile ("mrc p15, 1, %0, c15, c1, 0" : "=r" (regVal));  
    mvOsPrintf("Extra Feature Reg = 0x%x\n",regVal);

   __asm volatile ("mrc	p15, 0, %0, c1, c0, 0" : "=r" (regVal));  
   mvOsPrintf("Control Reg = 0x%x\n",regVal);

   __asm volatile ("mrc	p15, 0, %0, c0, c0, 0" : "=r" (regVal));  
    mvOsPrintf("ID Code Reg = 0x%x\n",regVal);

   __asm volatile ("mrc	p15, 0, %0, c0, c0, 1" : "=r" (regVal));  
   mvOsPrintf("Cache Type Reg = 0x%x\n",regVal);

}

MV_U32 mvCpuIfPrintSystemConfig(MV_8 *buffer, MV_U32 index)
{
  MV_U32 count = 0;
  
  MV_8 L2_ECC_str[MV_PROC_STR_SIZE];
  MV_8 L2_En_str[MV_PROC_STR_SIZE];
  MV_8 L2_Prefetch_str[MV_PROC_STR_SIZE];
  MV_8 Write_Alloc_str[MV_PROC_STR_SIZE];
  MV_8 Cpu_Stream_str[MV_PROC_STR_SIZE];
  
  mvCpuIfGetL2Mode(L2_En_str);
  mvCpuIfGetL2EccMode(L2_ECC_str); 
  mvCpuIfGetL2PrefetchMode(L2_Prefetch_str);
  mvCpuIfGetWriteAllocMode(Write_Alloc_str);
  mvCpuIfGetCpuStreamMode(Cpu_Stream_str);
  mvCpuIfPrintCpuRegs();
  
  count += mvOsSPrintf(buffer + count + index, "%s\n", L2_En_str);
  count += mvOsSPrintf(buffer + count + index, "%s\n", L2_ECC_str);
  count += mvOsSPrintf(buffer + count + index, "%s\n", L2_Prefetch_str);
  count += mvOsSPrintf(buffer + count + index, "%s\n", Write_Alloc_str);
  count += mvOsSPrintf(buffer + count + index, "%s\n", Cpu_Stream_str);
  return count;
}

MV_U32 whoAmI(MV_VOID)
{
	return 0;
}
