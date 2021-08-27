/** \file sys_service.c
 * sys_service.c
 *
 *  This file implements the required system services for the API-II using a
 * Linux OS running on the UVB.  The user should replace the functions provided
 * here with the equivalent based on their OS and hardware.
 *
 * Copyright (c) 2008, Zarlink Semiconductor, Inc.
 */
#include "vp_api_types.h"
#include "sys_service.h"
/*
 * VpSysWait() function implementation is needed only for CSLAC devices
 * (880, 790). For other devices this function could be commented.
 */
void
VpSysWait(
    uint8 time)  /* Time specified in increments of 125uS (e.g. 4 = 500uS) */
{
	mvOsUDelay(125 * time);
}
void VpSysSemaphoreInit(int semaphoreId)
{
}

/*
 * VpSysEnterCritical(), VpSysExitCritical():
 *
 *  These functions allow for disabling interrupts while executing nonreentrant
 * portions of VoicePath API code. Note that the following implementations of
 * enter/exit critical section functions are simple implementations. These
 * functions could be expanded (if required) to handle different critical
 * section types differently.
 *
 * Params:
 *  VpDeviceIdType deviceId: Device Id (chip select ID)
 *  VpCriticalSecType: Critical section type
 *
 * Return:
 *  Number of critical sections currently entered for the device.
 */
uint8
VpSysEnterCritical(
    VpDeviceIdType deviceId,
    VpCriticalSecType criticalSecType)
{
    /*
     * Code to check if semaphore can be taken, and if so increment. Block if
     * semaphore cannot be taken for this process ID.
     */
    /*
     * criticalDepth++;
     * return criticalDepth;
     */
    /* Prevent compiler from generating error */
    return 1;
} /* VpSysEnterCritical() */
uint8
VpSysExitCritical(
    VpDeviceIdType deviceId,
    VpCriticalSecType criticalSecType)
{
    /* Code to decrement semaphore */
    /*
     * criticalDepth--;
     * return criticalDepth;
     */
    /* Prevent compiler from generating error */
    return 0;
} /* VpSysExitCritical() */
/**
 * VpSysDisableInt(), VpSysEnableInt(), and VpSysTestInt()
 *
 *  These functions are used by the CSLAC device family for interrupt driven
 * polling modes. These are called by the API to detect when a non-masked
 * device status has changed.  If using SIMPLE_POLL mode, these functions do not
 * require implementation.
 *
 * Preconditions:
 *  None. The implementation of these functions is architecture dependent.
 *
 * Postconditions:
 *  VpSysDisableInt() - The interrupt associated with the deviceId passed is
 * disabled.
 *
 * VpSysEnableInt() - The interrupt associated with the deviceId passed is
 * enabled.
 *
 * VpSysTestInt() - The return value is TRUE if an interrupt occurred, otherwise
 * return FALSE.
 *
 * These functions are needed only for CSLAC devices
 * (880, 790). For other devices these functions could be commented.
 *
 */
void
VpSysDisableInt(
    VpDeviceIdType deviceId)
{
    mvSysTdmIntDisable((unsigned char)deviceId);
}
void
VpSysEnableInt(
    VpDeviceIdType deviceId)
{
    mvSysTdmIntEnable((unsigned char)deviceId);
}
bool
VpSysTestInt(
    VpDeviceIdType deviceId)
{
    return FALSE;
}
/**
 * VpSysDtmfDetEnable(), VpSysDtmfDetDisable()
 *
 *  These functions are used by the CSLAC device family for devices that do not
 * internally detect DTMF. It is used for Caller ID type-II and is provided to
 * enable external DTMF detection.
 *
 * Preconditions:
 *  None. The implementation of these functions is application dependent.
 *
 * Postconditions:
 *  VpSysDtmfDetEnable() - The device/channel resource for DTMF detection is
 * enabled.
 *
 *  VpSysDtmfDetDisable() - The device/channel resource for DTMF detection is
 * disabled.
 *
 * These functions are needed only for CSLAC devices
 * (880, 790). For other devices these functions could be commented.
 *
 */
void
VpSysDtmfDetEnable(
    VpDeviceIdType deviceId,
    uint8 channelId)
{
}
void
VpSysDtmfDetDisable(
    VpDeviceIdType deviceId,
    uint8 channelId)
{
}
/*
 * The following functions VpSysTestHeapAcquire(),  VpSysTestHeapRelease()
 * VpSysPcmCollectAndProcess() and are needed only for CSLAC devices
 * (880). For other devices these functions could be commented. Please see
 * the LineTest API documentation for function requirements.
 */
void *
VpSysTestHeapAcquire(
    uint8 *pHeapId)
{
    return VP_NULL;
} /* VpSysTestHeapAcquire() */
bool
VpSysTestHeapRelease(
    uint8 heapId)
{
    return TRUE;
} /* VpSysTestHeapRelease() */
void
VpSysPcmCollectAndProcess(
    void *pLineCtx,
    VpDeviceIdType deviceId,
    uint8 channelId,
    uint8 startTimeslot,
    uint16 operationTime,
    uint16 settlingTime,
    uint16 operationMask)
{
} /* VpSysPcmCollectAndProcess() */

#ifdef ENABLE_DBG_TAG
int
VpSysDebugPrintf(
    const char *format, ...)
{
    va_list ap;
    static char buf[1024];
    static uint16 indexBuf;
    static bool beginLine = TRUE;
    uint16 bufLen;
    int retval;

    if (beginLine == TRUE) {
	mvOsPrintf("<DBG> ");
	beginLine = FALSE;
    }
    /* print in a string to check if there's a \n */
    va_start(ap, format);
    retval = vsprintf(&buf[indexBuf], format, ap);
    va_end(ap);

    bufLen = strlen(&buf[indexBuf]);
    if (buf[indexBuf + bufLen - 1] == '\n') {
		buf[indexBuf + bufLen - 1] = ' ';
		retval = mvOsPrintf("%s</DBG>\n", buf);
		indexBuf = 0;
		beginLine = TRUE;
    } else {
		indexBuf += bufLen;

		/* Just in case of a buffer overflow, not suppose to append */
		if (indexBuf > 800) {
			retval = mvOsPrintf("%s</DBG>\n", buf);
			indexBuf = 0;
			beginLine = TRUE;
		}
      }
    return retval;
}
#endif /* ENABLE_DBG_TAG */
