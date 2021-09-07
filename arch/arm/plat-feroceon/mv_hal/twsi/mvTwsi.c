#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvTwsi.h"
#include "mvTwsiSpec.h"
#include "cpu/mvCpu.h"

#ifdef MV_DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

static MV_VOID twsiIntFlgClr(MV_U8 chanNum);
static MV_BOOL twsiMainIntGet(MV_U8 chanNum);
static MV_VOID twsiAckBitSet(MV_U8 chanNum);
static MV_U32 twsiStsGet(MV_U8 chanNum);
static MV_VOID twsiReset(MV_U8 chanNum);
static MV_STATUS twsiAddr7BitSet(MV_U8 chanNum, MV_U32 deviceAddress,MV_TWSI_CMD command);
static MV_STATUS twsiAddr10BitSet(MV_U8 chanNum, MV_U32 deviceAddress,MV_TWSI_CMD command);
static MV_STATUS twsiDataTransmit(MV_U8 chanNum, MV_U8 *pBlock, MV_U32 blockSize);
static MV_STATUS twsiDataReceive(MV_U8 chanNum, MV_U8 *pBlock, MV_U32 blockSize);
static MV_STATUS twsiTargetOffsSet(MV_U8 chanNum, MV_U32 offset,MV_BOOL moreThen256);

static MV_BOOL twsiTimeoutChk(MV_U32 timeout, const MV_8 *pString)
{
	if(timeout >= TWSI_TIMEOUT_VALUE)
	{
		DB(mvOsPrintf("%s",pString));
		return MV_TRUE;
	}
	return MV_FALSE;
	
}
 
MV_STATUS mvTwsiStartBitSet(MV_U8 chanNum)
{
	MV_BOOL isIntFlag = MV_FALSE;
	MV_U32 timeout, temp;

	DB(mvOsPrintf("TWSI: mvTwsiStartBitSet \n"));
	 
    	if(twsiMainIntGet(chanNum))
		isIntFlag = MV_TRUE;
	 
    	temp = MV_REG_READ(TWSI_CONTROL_REG(chanNum));
	MV_REG_WRITE(TWSI_CONTROL_REG(chanNum), temp | TWSI_CONTROL_START_BIT);
	
	if(isIntFlag){
		DB(mvOsPrintf("TWSI: mvTwsiStartBitSet repeated start Bit\n"));
		twsiIntFlgClr(chanNum);
	}
	
	timeout = 0;
	while(!twsiMainIntGet(chanNum) && (timeout++ < TWSI_TIMEOUT_VALUE));
	
	if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: mvTwsiStartBitSet ERROR - Start Clear bit TimeOut .\n"))
		return MV_TIMEOUT;

	if((MV_REG_READ(TWSI_CONTROL_REG(chanNum)) & TWSI_CONTROL_START_BIT) != 0)
	{
		mvOsPrintf("TWSI: mvTwsiStartBitSet ERROR - start bit didn't went down\n");
		return MV_FAIL;
	}	

	temp = twsiStsGet(chanNum);
	if(( temp != TWSI_START_CON_TRA ) && ( temp != TWSI_REPEATED_START_CON_TRA ))
	  {
		mvOsPrintf("TWSI: mvTwsiStartBitSet ERROR - status %x after Set Start Bit. \n",temp);
		return MV_FAIL;
	}

	return MV_OK;	

}

MV_STATUS mvTwsiStopBitSet(MV_U8 chanNum)
{
    	MV_U32	timeout, temp;

	temp = MV_REG_READ(TWSI_CONTROL_REG(chanNum));
    	MV_REG_WRITE(TWSI_CONTROL_REG(chanNum), temp | TWSI_CONTROL_STOP_BIT);

	twsiIntFlgClr(chanNum);
		
	timeout = 0;
	while( ((MV_REG_READ(TWSI_CONTROL_REG(chanNum)) & TWSI_CONTROL_STOP_BIT) != 0) && (timeout++ < TWSI_TIMEOUT_VALUE));

	if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: mvTwsiStopBitSet ERROR - Stop bit TimeOut .\n"))
		return MV_TIMEOUT;
	
	if((MV_REG_READ(TWSI_CONTROL_REG(chanNum)) & TWSI_CONTROL_STOP_BIT) != 0)	
	{
		mvOsPrintf("TWSI: mvTwsiStopBitSet ERROR - stop bit didn't went down. \n");
		return MV_FAIL;
	}
	
	temp = twsiStsGet(chanNum);
	if( temp != TWSI_NO_REL_STS_INT_FLAG_IS_KEPT_0){
		mvOsPrintf("TWSI: mvTwsiStopBitSet ERROR - status %x after Stop Bit. \n", temp);
		return MV_FAIL;
	}

	return MV_OK;
}

static MV_BOOL twsiMainIntGet(MV_U8 chanNum)
{
	MV_U32 temp;
	
	temp = MV_REG_READ(TWSI_CPU_MAIN_INT_CAUSE_REG(chanNum));
	if (temp & (TWSI_CPU_MAIN_INT_BIT(chanNum)))
	    return MV_TRUE;
    
	return MV_FALSE;
}
 
static MV_VOID twsiIntFlgClr(MV_U8 chanNum)
{
	MV_U32 temp;

   	mvOsDelay(1);
	 
	temp = MV_REG_READ(TWSI_CONTROL_REG(chanNum));
    	MV_REG_WRITE(TWSI_CONTROL_REG(chanNum),temp & ~(TWSI_CONTROL_INT_FLAG_SET));

   	mvOsDelay(1);
	
	return;
}

static MV_VOID twsiAckBitSet(MV_U8 chanNum)
{
	MV_U32 temp;

	temp = MV_REG_READ(TWSI_CONTROL_REG(chanNum));
    	MV_REG_WRITE(TWSI_CONTROL_REG(chanNum), temp | TWSI_CONTROL_ACK);

	mvOsDelay(1);
	return;
}

MV_U32 mvTwsiInit(MV_U8 chanNum, MV_HZ frequancy, MV_U32 Tclk, MV_TWSI_ADDR *pTwsiAddr, MV_BOOL generalCallEnable)
{
    	MV_U32	n,m,freq,margin,minMargin = 0xffffffff;
	MV_U32	power;
    	MV_U32	actualFreq = 0,actualN = 0,actualM = 0,val;

	if(frequancy > 100000)
	{
		mvOsPrintf("Warning TWSI frequancy is too high, please use up tp 100Khz. \n");
	}

	DB(mvOsPrintf("TWSI: mvTwsiInit - Tclk = %d freq = %d\n",Tclk,frequancy));
    	 
    	for(n = 0 ; n < 8 ; n++)
    	{
        	for(m = 0 ; m < 16 ; m++)
        	{
            		power = 2 << n;  
            		freq = Tclk/(10*(m+1)*power);
            		margin = MV_ABS(frequancy - freq);
            		if(margin < minMargin)
            		{
                		minMargin   = margin;
                		actualFreq  = freq;
                		actualN     = n;
                		actualM     = m;
            		}
        	}
		}
	DB(mvOsPrintf("TWSI: mvTwsiInit - actN %d actM %d actFreq %d\n",actualN , actualM, actualFreq));
	 
	twsiReset(chanNum);

	val = ((actualM<< TWSI_BAUD_RATE_M_OFFS) | actualN << TWSI_BAUD_RATE_N_OFFS);
    	MV_REG_WRITE(TWSI_STATUS_BAUDE_RATE_REG(chanNum),val);

	MV_REG_WRITE(TWSI_CONTROL_REG(chanNum), TWSI_CONTROL_ENA | TWSI_CONTROL_ACK); 

	if( pTwsiAddr->type == ADDR10_BIT ) 
    	{
		 
		val = ((pTwsiAddr->address & TWSI_SLAVE_ADDR_10BIT_MASK) >> TWSI_SLAVE_ADDR_10BIT_OFFS );
		 
		val |= TWSI_SLAVE_ADDR_10BIT_CONST;
		 
		if(generalCallEnable)
			val |= TWSI_SLAVE_ADDR_GCE_ENA;
		 
		MV_REG_WRITE(TWSI_SLAVE_ADDR_REG(chanNum),val);

        	val = (pTwsiAddr->address << TWSI_EXTENDED_SLAVE_OFFS) & TWSI_EXTENDED_SLAVE_MASK;  
        	MV_REG_WRITE(TWSI_EXTENDED_SLAVE_ADDR_REG(chanNum), val);
    	}
    	else  
    	{
		 
        	MV_REG_WRITE(TWSI_EXTENDED_SLAVE_ADDR_REG(chanNum),0x0);
		val = (pTwsiAddr->address << TWSI_SLAVE_ADDR_7BIT_OFFS) & TWSI_SLAVE_ADDR_7BIT_MASK;
        	MV_REG_WRITE(TWSI_SLAVE_ADDR_REG(chanNum), val);
    	}

    val = MV_REG_READ(TWSI_CONTROL_REG(chanNum));
	MV_REG_WRITE(TWSI_CONTROL_REG(chanNum), val | TWSI_CONTROL_INT_ENA);
	 
	mvOsDelay(1);
	
   return actualFreq;
} 

static MV_U32 twsiStsGet(MV_U8 chanNum)
{
    return MV_REG_READ(TWSI_STATUS_BAUDE_RATE_REG(chanNum));

}

static MV_VOID twsiReset(MV_U8 chanNum)
{
    	 
    	MV_REG_WRITE(TWSI_SOFT_RESET_REG(chanNum),0);

   	mvOsDelay(2);

	return;
}

MV_STATUS mvTwsiAddrSet(MV_U8 chanNum, MV_TWSI_ADDR *pTwsiAddr, MV_TWSI_CMD command)
{
	DB(mvOsPrintf("TWSI: mvTwsiAddr7BitSet addr %x , type %d, cmd is %s\n",pTwsiAddr->address,\
		 			pTwsiAddr->type, ((command==MV_TWSI_WRITE)?"Write":"Read") ));
	 
	if(pTwsiAddr->type == ADDR10_BIT)
	{
		return twsiAddr10BitSet(chanNum, pTwsiAddr->address,command);
	}
	 
	else
	{
		return twsiAddr7BitSet(chanNum, pTwsiAddr->address,command);
	}

}

static MV_STATUS twsiAddr10BitSet(MV_U8 chanNum, MV_U32 deviceAddress,MV_TWSI_CMD command)
{
	MV_U32 val,timeout;

	val = ((deviceAddress & TWSI_DATA_ADDR_10BIT_MASK) >> TWSI_DATA_ADDR_10BIT_OFFS );
	 
	val |= TWSI_DATA_ADDR_10BIT_CONST;
	 
	val |= command;
	MV_REG_WRITE(TWSI_DATA_REG(chanNum), val);
	 
	mvOsDelay(1);

	twsiIntFlgClr(chanNum);

	timeout = 0;
	while( !twsiMainIntGet(chanNum) && (timeout++ < TWSI_TIMEOUT_VALUE));

	if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiAddr10BitSet ERROR - 1st addr (10Bit) Int TimeOut.\n"))
		return MV_TIMEOUT;
	
	val = twsiStsGet(chanNum);
	if(( (val != TWSI_AD_PLS_RD_BIT_TRA_ACK_REC) && (command == MV_TWSI_READ ) ) || 
	   ( (val != TWSI_AD_PLS_WR_BIT_TRA_ACK_REC) && (command == MV_TWSI_WRITE) ))
	{
		mvOsPrintf("TWSI: twsiAddr10BitSet ERROR - status %x 1st addr (10 Bit) in %s mode.\n"\
						,val, ((command==MV_TWSI_WRITE)?"Write":"Read") );
		return MV_FAIL;
	}

	val = (deviceAddress << TWSI_DATA_ADDR_7BIT_OFFS) & TWSI_DATA_ADDR_7BIT_MASK;
	MV_REG_WRITE(TWSI_DATA_REG(chanNum), val);

	twsiIntFlgClr(chanNum);

	timeout = 0;
	while( !twsiMainIntGet(chanNum) && (timeout++ < TWSI_TIMEOUT_VALUE));

	if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiAddr10BitSet ERROR - 2nd (10 Bit) Int TimOut.\n"))
		return MV_TIMEOUT;
	
	val = twsiStsGet(chanNum);
	if(( (val != TWSI_SEC_AD_PLS_RD_BIT_TRA_ACK_REC) && (command == MV_TWSI_READ ) ) || 
	   ( (val != TWSI_SEC_AD_PLS_WR_BIT_TRA_ACK_REC) && (command == MV_TWSI_WRITE) ))
	{
		mvOsPrintf("TWSI: twsiAddr10BitSet ERROR - status %x 2nd addr(10 Bit) in %s mode.\n"\
						,val, ((command==MV_TWSI_WRITE)?"Write":"Read") );
		return MV_FAIL;
	}
	
	return MV_OK;
}

static MV_STATUS twsiAddr7BitSet(MV_U8 chanNum, MV_U32 deviceAddress,MV_TWSI_CMD command)
{
	MV_U32 val,timeout;

	val = (deviceAddress << TWSI_DATA_ADDR_7BIT_OFFS) & TWSI_DATA_ADDR_7BIT_MASK;
	 
	val |= command;	
	MV_REG_WRITE(TWSI_DATA_REG(chanNum), val);
	 
	mvOsDelay(1);

	twsiIntFlgClr(chanNum);

	timeout = 0;
	while( !twsiMainIntGet(chanNum) && (timeout++ < TWSI_TIMEOUT_VALUE));

	if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiAddr7BitSet ERROR - Addr (7 Bit) int TimeOut.\n"))
		return MV_TIMEOUT;
	
	val = twsiStsGet(chanNum);
	if(( (val != TWSI_AD_PLS_RD_BIT_TRA_ACK_REC) && (command == MV_TWSI_READ ) ) || 
	   ( (val != TWSI_AD_PLS_WR_BIT_TRA_ACK_REC) && (command == MV_TWSI_WRITE) ))
	{
		 
		DB(mvOsPrintf("TWSI: twsiAddr7BitSet ERROR - status %x addr (7 Bit) in %s mode.\n"\
						,val,((command==MV_TWSI_WRITE)?"Write":"Read") ));
		return MV_FAIL;
	}
	
	return MV_OK;
}

static MV_STATUS twsiDataTransmit(MV_U8 chanNum, MV_U8 *pBlock, MV_U32 blockSize)
{
	MV_U32 timeout, temp, blockSizeWr = blockSize;

	if(NULL == pBlock)
		return MV_BAD_PARAM;

	timeout = 0;
	while( !twsiMainIntGet(chanNum) && (timeout++ < TWSI_TIMEOUT_VALUE));

	if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiDataTransmit ERROR - Read Data Int TimeOut.\n"))
		return MV_TIMEOUT;

	while(blockSizeWr)
	{
		 
		MV_REG_WRITE(TWSI_DATA_REG(chanNum),(MV_U32)*pBlock);
		DB(mvOsPrintf("TWSI: twsiDataTransmit place = %d write %x \n",\
						blockSize - blockSizeWr, *pBlock));
		pBlock++;
		blockSizeWr--;

		twsiIntFlgClr(chanNum);

		timeout = 0;
		while( !twsiMainIntGet(chanNum) && (timeout++ < TWSI_TIMEOUT_VALUE));

		if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiDataTransmit ERROR - Read Data Int TimeOut.\n"))
			return MV_TIMEOUT;

		temp = twsiStsGet(chanNum);
		if(temp != TWSI_M_TRAN_DATA_BYTE_ACK_REC) 
		{
			mvOsPrintf("TWSI: twsiDataTransmit ERROR - status %x in write trans\n",temp);
			return MV_FAIL;
		}
		
	}

	return MV_OK;
}

static MV_STATUS twsiDataReceive(MV_U8 chanNum, MV_U8 *pBlock, MV_U32 blockSize)
{
	MV_U32 timeout, temp, blockSizeRd = blockSize;
	if(NULL == pBlock)
		return MV_BAD_PARAM;

	timeout = 0;
	while( !twsiMainIntGet(chanNum) && (timeout++ < TWSI_TIMEOUT_VALUE));

	if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiDataReceive ERROR - Read Data int Time out .\n"))
		return MV_TIMEOUT;

	while(blockSizeRd)
	{
		if(blockSizeRd == 1)
		{
			 
			temp = MV_REG_READ(TWSI_CONTROL_REG(chanNum));
			temp &=  ~(TWSI_CONTROL_ACK);
			MV_REG_WRITE(TWSI_CONTROL_REG(chanNum), temp);
		}
		twsiIntFlgClr(chanNum);
		 
		timeout = 0;
		while( (!twsiMainIntGet(chanNum)) && (timeout++ < TWSI_TIMEOUT_VALUE));

		if(MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiDataReceive ERROR - Read Data Int Time out .\n"))
			return MV_TIMEOUT;

		temp = twsiStsGet(chanNum);
		if((temp != TWSI_M_REC_RD_DATA_ACK_TRA) && (blockSizeRd !=1))
		{
			mvOsPrintf("TWSI: twsiDataReceive ERROR - status %x in read trans \n",temp);
			return MV_FAIL;
		}
		else if((temp != TWSI_M_REC_RD_DATA_ACK_NOT_TRA) && (blockSizeRd ==1))
		{
			mvOsPrintf("TWSI: twsiDataReceive ERROR - status %x in Rd Terminate\n",temp);
			return MV_FAIL;
		}
		
		*pBlock = (MV_U8)MV_REG_READ(TWSI_DATA_REG(chanNum));
		DB(mvOsPrintf("TWSI: twsiDataReceive  place %d read %x \n",\
						blockSize - blockSizeRd,*pBlock));
		pBlock++;
		blockSizeRd--;
	}

	return MV_OK;
}

static MV_STATUS twsiTargetOffsSet(MV_U8 chanNum, MV_U32 offset, MV_BOOL moreThen256)
{
	MV_U8 offBlock[2];
	MV_U32 offSize;

	if(moreThen256 == MV_TRUE)
	{
		offBlock[0] = (offset >> 8) & 0xff;
		offBlock[1] = offset & 0xff;
		offSize = 2;
	}
	else
	{
		offBlock[0] = offset & 0xff;
		offSize = 1;
	}
	DB(mvOsPrintf("TWSI: twsiTargetOffsSet offSize = %x addr1 = %x addr2 = %x\n",\
							offSize,offBlock[0],offBlock[1]));
	return twsiDataTransmit(chanNum, offBlock, offSize);

}

MV_STATUS mvTwsiRead(MV_U8 chanNum, MV_TWSI_SLAVE *pTwsiSlave, MV_U8 *pBlock, MV_U32 blockSize)
{
	if((NULL == pBlock) || (NULL == pTwsiSlave))
		return MV_BAD_PARAM;
	if(MV_OK != mvTwsiStartBitSet(chanNum))
	{
		mvTwsiStopBitSet(chanNum);
		 return MV_FAIL;
	}
	
	DB(mvOsPrintf("TWSI: mvTwsiEepromRead after mvTwsiStartBitSet\n"));
	
	if(MV_TRUE == pTwsiSlave->validOffset)
	{
		if(MV_OK != mvTwsiAddrSet(chanNum, &(pTwsiSlave->slaveAddr), MV_TWSI_WRITE)) 
		{
			mvTwsiStopBitSet(chanNum);
			return MV_FAIL;
		} 
		DB(mvOsPrintf("TWSI: mvTwsiEepromRead after mvTwsiAddrSet\n"));
		if(MV_OK != twsiTargetOffsSet(chanNum, pTwsiSlave->offset, pTwsiSlave->moreThen256)) 
		{
			mvTwsiStopBitSet(chanNum);
			return MV_FAIL;
		}
		DB(mvOsPrintf("TWSI: mvTwsiEepromRead after twsiTargetOffsSet\n"));
		if(MV_OK != mvTwsiStartBitSet(chanNum)) 
		{
			mvTwsiStopBitSet(chanNum);
			return MV_FAIL;
		}
		DB(mvOsPrintf("TWSI: mvTwsiEepromRead after mvTwsiStartBitSet\n"));
	}
	if(MV_OK != mvTwsiAddrSet(chanNum, &(pTwsiSlave->slaveAddr), MV_TWSI_READ)) 
	{
		mvTwsiStopBitSet(chanNum);
		return MV_FAIL;
	} 
	DB(mvOsPrintf("TWSI: mvTwsiEepromRead after mvTwsiAddrSet\n"));
	if(MV_OK != twsiDataReceive(chanNum, pBlock, blockSize))
	{
		mvTwsiStopBitSet(chanNum);
		return MV_FAIL;
	}
	DB(mvOsPrintf("TWSI: mvTwsiEepromRead after twsiDataReceive\n"));

	if(MV_OK != mvTwsiStopBitSet(chanNum))
	{
		return MV_FAIL;
	}

	twsiAckBitSet(chanNum);

	DB(mvOsPrintf("TWSI: mvTwsiEepromRead after mvTwsiStopBitSet\n"));

	return MV_OK;
}
#ifdef MY_ABC_HERE
EXPORT_SYMBOL(mvTwsiRead);
#endif

MV_STATUS mvTwsiWrite(MV_U8 chanNum, MV_TWSI_SLAVE *pTwsiSlave, MV_U8 *pBlock, MV_U32 blockSize)
{
	if((NULL == pBlock) || (NULL == pTwsiSlave))
		return MV_BAD_PARAM;

	if(MV_OK != mvTwsiStartBitSet(chanNum)) 
	{
		mvTwsiStopBitSet(chanNum);
		return MV_FAIL;
	}

	DB(mvOsPrintf("TWSI: mvTwsiEepromWrite after mvTwsiStartBitSet\n"));
	if(MV_OK != mvTwsiAddrSet(chanNum, &(pTwsiSlave->slaveAddr), MV_TWSI_WRITE))
	{
		mvTwsiStopBitSet(chanNum);
		return MV_FAIL;
	}
	DB(mvOsPrintf("TWSI :mvTwsiEepromWrite after mvTwsiAddrSet\n"));

	if(MV_TRUE == pTwsiSlave->validOffset)
	{
		if(MV_OK != twsiTargetOffsSet(chanNum, pTwsiSlave->offset, pTwsiSlave->moreThen256)) 
		{
			mvTwsiStopBitSet(chanNum);
			return MV_FAIL;
		}
		DB(mvOsPrintf("TWSI: mvTwsiEepromWrite after twsiTargetOffsSet\n"));
	}
	if(MV_OK != twsiDataTransmit(chanNum, pBlock, blockSize)) 
	{
		mvTwsiStopBitSet(chanNum);
		return MV_FAIL;
	}
	DB(mvOsPrintf("TWSI: mvTwsiEepromWrite after twsiDataTransmit\n"));
	if(MV_OK != mvTwsiStopBitSet(chanNum)) 
	{
		return MV_FAIL;
	}
	DB(mvOsPrintf("TWSI: mvTwsiEepromWrite after mvTwsiStopBitSet\n"));

	return MV_OK;
}
#ifdef MY_ABC_HERE
EXPORT_SYMBOL(mvTwsiWrite);
#endif
