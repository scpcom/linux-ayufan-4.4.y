/** \file vp792_api.h
 * vp792_api.h
 *
 *  Header file that define all the commands for the Vp792 device.
 *
 * Copyright (c) 2010, Zarlink Semiconductor, Inc.
 *
 * $Revision: 7167 $
 * $LastChangedDate: 2010-05-13 19:04:07 -0500 (Thu, 13 May 2010) $
 */

#ifndef VP792_API_H
#define VP792_API_H

#define VP792_MIN_SLAC_ID       0
#define VP792_MAX_SLAC_ID       7

#define VP792_MAX_OUTSTANDING_RESPONSES         10

/* Maximum number of profiles of each type in the Profile Table: */
#define VP_792_DEV_PROF_TABLE_SIZE              1
#define VP_792_AC_PROF_TABLE_SIZE               2
#define VP_792_DC_PROF_TABLE_SIZE               2
#define VP_792_RINGING_PROF_TABLE_SIZE          2
#define VP_792_TONE_CADENCE_PROF_TABLE_SIZE     11
#define VP_792_TONE_PROF_TABLE_SIZE             10
#define VP_792_RING_CADENCE_PROF_TABLE_SIZE     4
#define VP_792_METERING_PROF_TABLE_SIZE         2
#define VP_792_CALLERID_PROF_TABLE_SIZE         2

/* Maximum length of each type of profile (bytes): */
#define VP792_DEV_PROFILE_LEN               40
#define VP792_DC_PROFILE_LEN                44
#define VP792_AC_PROFILE_LEN                128
#define VP792_CID_PROFILE_LEN               200
#define VP792_MTR_PROFILE_LEN               22
#define VP792_TONE_PROFILE_LEN              32
#define VP792_RINGCAD_PROFILE_LEN           72
#define VP792_TONECAD_PROFILE_LEN           72
#define VP792_RING_PROFILE_LEN              38

/* Lengths of profile sections that need to be stored or otherwise handled
 * separately from the whole. (words) */
#define VP792_AC_PROFILE_REGDATA_LEN        1
#define VP792_DC_PROFILE_REGDATA_LEN        22
#define VP792_RING_PROFILE_REGDATA_LEN      34

#define VP792_CID_TONE_DETECTED     0xFF
#define VP792_CID_TONE_UNDETECTED   0xFE

#define VP792_CRIT_FLT_EN_AC_BIT      0
#define VP792_CRIT_FLT_EN_DC_BIT      1
#define VP792_CRIT_FLT_EN_THERM_BIT   2

typedef enum
{
  VP_792_SEQ_OFF,
  VP_792_SEQ_RING,
  VP_792_SEQ_TONE,
  VP_792_SEQ_OTHER
} Vp792SequencerUseType;

#include "vp_hal.h"
#include "vp_CSLAC_types.h"

/* Struct for the Profile Table: */
typedef struct {
#define VP792_NUM_PROFILE_TYPES 9
    uint16 valid[VP792_NUM_PROFILE_TYPES];

#ifdef VP_COMMON_ADDRESS_SPACE
    /* We only need to save profile pointers in the device object. */
    VpProfilePtrType pDevice[VP_792_DEV_PROF_TABLE_SIZE];
    VpProfilePtrType pAc[VP_792_AC_PROF_TABLE_SIZE];
    VpProfilePtrType pDc[VP_792_DC_PROF_TABLE_SIZE];
    VpProfilePtrType pRinging[VP_792_RINGING_PROF_TABLE_SIZE];
    VpProfilePtrType pRingCad[VP_792_RING_CADENCE_PROF_TABLE_SIZE];
    VpProfilePtrType pTone[VP_792_TONE_PROF_TABLE_SIZE];
    VpProfilePtrType pMetering[VP_792_METERING_PROF_TABLE_SIZE];
    VpProfilePtrType pCallerId[VP_792_CALLERID_PROF_TABLE_SIZE];
    VpProfilePtrType pToneCad[VP_792_TONE_CADENCE_PROF_TABLE_SIZE];
#else
    /* Profiles must be copied into the device object. */
    VpProfileDataType device[VP_792_DEV_PROF_TABLE_SIZE][VP792_DEV_PROFILE_LEN];
    VpProfileDataType ac[VP_792_AC_PROF_TABLE_SIZE][VP792_AC_PROFILE_LEN];
    VpProfileDataType dc[VP_792_DC_PROF_TABLE_SIZE][VP792_DC_PROFILE_LEN];
    VpProfileDataType ringing[VP_792_RINGING_PROF_TABLE_SIZE][VP792_RING_PROFILE_LEN];
    VpProfileDataType ringCad[VP_792_RING_CADENCE_PROF_TABLE_SIZE][VP792_RINGCAD_PROFILE_LEN];
    VpProfileDataType tone[VP_792_TONE_PROF_TABLE_SIZE][VP792_TONE_PROFILE_LEN];
    VpProfileDataType metering[VP_792_METERING_PROF_TABLE_SIZE][VP792_MTR_PROFILE_LEN];
    VpProfileDataType callerId[VP_792_CALLERID_PROF_TABLE_SIZE][VP792_CID_PROFILE_LEN];
    VpProfileDataType toneCad[VP_792_TONE_CADENCE_PROF_TABLE_SIZE][VP792_TONECAD_PROFILE_LEN];
#endif

} Vp792ProfileTableType;

typedef enum
{
  VP792_REQUEST_SET_REL_GAIN,
  VP792_REQUEST_GET_LOOP_COND,
  VP792_REQUEST_GET_OPTION,
  VP792_REQUEST_DEVICE_IO_ACCESS_EXT,
  VP792_REQUEST_LINE_IO_ACCESS,
  VP792_REQUEST_QUERY,
  VP792_REQUEST_LOW_LEVEL_CMD_16,
  VP792_REQUEST_SENDSIG_MSGWAIT,
  VP792_REQUEST_TEST_INTERNAL,
  VP792_REQUEST_TEST_CMP,
  VP792_REQUEST_TIMER_CMP
} Vp792RequestTagType;

/* Struct for information about pending responses (used in
VpGetResults()): */
typedef struct {
    bool outstanding;

    Vp792RequestTagType requestType;

    union {
        struct {
            uint16 handle;
            uint8 channelId;
        } setRelGain;

        struct {
            uint16 handle;
        } getLoopCond;

        struct {
            uint16 handle;
            uint8 channelId;
            VpOptionIdType optionId;
        } getOption;

        struct {
            VpDeviceIoAccessExtType deviceIoAccess;
        } deviceIoAccessExt;

        struct {
            uint16 handle;
            VpLineIoAccessType lineIoAccess;
        } lineIoAccess;

        struct {
            uint16 handle;
            VpQueryIdType queryId;
        } query;

        struct {
            uint16 handle;
            uint8 channelId;
            VpLowLevelCmdType cmdType;
            uint16 writeWords[2];
            uint8 numWriteWords;
            uint8 numReadWords;
        } lowLevelCmd16;

        struct {
            uint16 eventId;
            uint16 handle;
            uint8 channelId;
            uint16 respSize;
            uint16 cmdId;
        } testLine;

        struct {
            uint16 handle;
            uint8 channelId;
            bool internal;
        } genTimer;
    } args;
} Vp792ResponseRequestType;

typedef struct {
    uint32 opaqueData[2400];
} Vp792OpaquePmDevObjType;

/* Vp792-specific Device Object */
typedef struct {
    /* Customer-defined device identifier struct, passed to HAL functions: */
    VpDeviceIdType deviceId;

    /* Slac identifier mapped with VpMapSlacId().  This member allows for
       multiple device objects to share a common deviceId (which is the case
       when they share a chip-select on the SPI bus). */
    uint8 slacId;

    /* Total number of channels controlled by this device: */
    uint8 maxChannels;

    /* SLAC revision info */
    struct {
        uint16 device;
        uint16 product;
        uint16 version;
        uint32 patchAddress;
    } rev;

    /* A flag indicating whether VpInitDevice() has completed: */
    bool devInit;

    /* The Profile Table: */
    Vp792ProfileTableType profTable;

    /* PCM Clock Rate in units of 1KHz */
    uint16 pcmClockRate;

    /* High bits of the VP-API-II timestamp.  We maintain the high bits here
     because the SLAC's timestamp has finer granularity. */
    uint16 timeStampHiBits;

    /* Struct for locally-maintained device-specific options: */
    struct {
        VpOptionCriticalFltType criticalFlt;
        VpOptionEventMaskType eventMask;
        uint16 pulseEnabled;
        uint16 dtmfEnabled;
        VpOptionParkModeType parkMode;
        uint32 debugSelect;
    } options;

    /* Temporary storage for interrupt indication, while command mailbox is busy. */
    uint16 intInd[2];

    /* Array for keeping track of outstanding response requests: */
    Vp792ResponseRequestType responseRequest[VP792_MAX_OUTSTANDING_RESPONSES];

    /* Index (in the above array) of the next result to be returned by
       VpGetResults(): */
    uint16 requestIdx;

    /* SLAC response mailbox cache (temporary storage used in cases of mailbox
       congestion, where we need to empty the response mailbox before we can
       issue another command in the command mailbox): */
    struct {
        uint8 count;            /* When the cache is nonempty, count > 0. */
#define VP792_MAX_RSPSIZE 42
        uint16 data[2][VP792_MAX_RSPSIZE];
    } respMboxCache;

    /* Bitmask indicating which device-specific event handlers are running. */
    uint16 eventHandlers;

    /* State variables for EhInitDevice() event handler: */
    struct {

#ifdef VP_COMMON_ADDRESS_SPACE
        /* We only need to save profile pointers in the line object. */
        VpProfilePtrType pDevProfile;
        VpProfilePtrType pAcProfile;
        VpProfilePtrType pDcProfile;
        VpProfilePtrType pRingProfile;
#else
        /* Profiles must be copied into the line object. */
        uint16 profilesValid;
        VpProfileDataType devProfile[VP792_DEV_PROFILE_LEN];
        VpProfileDataType acProfile[VP792_AC_PROFILE_LEN];
        VpProfileDataType dcProfile[VP792_DC_PROFILE_LEN];
        VpProfileDataType ringProfile[VP792_RING_PROFILE_LEN];
#endif

    } ehInitDevice;

#ifdef VP792_INCLUDE_TESTLINE_CODE
    Vp792OpaquePmDevObjType opaquePmDevObj;
#endif

} Vp792DeviceObjectType;

/* Vp792-specific Line Object */
typedef struct
{

    /* Number used by the Vp792 to uniquely identify this line: */
    uint8 channelId;

    /* Line termination type: */
    VpTermType termType;

    /* Customer-defined line identifier struct, returned in the event
    * structure by VpGetEvent() when an event occurs on this line: */
    VpLineIdType lineId;

    /* A flag indicating whether VpInitLine() has completed: */
    bool lineInit;

    /* Struct for locally-maintained line-specific options: */
    struct {
        VpOptionDtmfModeControlType dtmfControlMode;
        VpOptionZeroCrossType zeroCross;
        VpOptionPulseModeType pulseMode;
        VpOptionEventMaskType eventMask;
        VpOptionRingControlType ringControl;
        VpOptionPcmTxRxCntrlType pcmTxRxCntrl;
        VpOptionLoopbackType loopback;
        uint16 afeConfig; /* cached AFE_CONFIG value for loopback option */
        uint32 debugSelect;
    } options;

    struct {
#ifdef VP_COMMON_ADDRESS_SPACE
        /* We only need to save profile pointers in the line object. */
        VpProfilePtrType pDevice;
        VpProfilePtrType pAc;
        VpProfilePtrType pDc;
        VpProfilePtrType pRinging;
        VpProfilePtrType pRingCad;
        VpProfilePtrType pTone;
        VpProfilePtrType pMetering;
        VpProfilePtrType pCallerId;
        VpProfilePtrType pToneCad;
#else
        /* Profiles must be copied into the line object. */
        uint16 valid;
        VpProfileDataType device[VP792_DEV_PROFILE_LEN];
        VpProfileDataType ac[VP792_AC_PROFILE_LEN];
        VpProfileDataType dc[VP792_DC_PROFILE_LEN];
        VpProfileDataType ringing[VP792_RING_PROFILE_LEN];
        VpProfileDataType ringCad[VP792_RINGCAD_PROFILE_LEN];
        VpProfileDataType tone[VP792_TONE_PROFILE_LEN];
        VpProfileDataType metering[VP792_MTR_PROFILE_LEN];
        VpProfileDataType callerId[VP792_CID_PROFILE_LEN];
        VpProfileDataType toneCad[VP792_TONECAD_PROFILE_LEN];
#endif

    } profiles;

    /* Bitmask indicating which line-specific event handlers are running. */
    uint16 eventHandlers;

    /* State variables for EhSetLineState() event handler: */
    struct {
        bool waitForMeterAbort;
        bool waitForSequencerReady;
        VpLineStateType newState;
    } ehSetLineState;

    /* Current API line state */
    VpLineStateType currentState;

    /* Currently-detected ongoing DTMF digit (if any): */
    uint16 dtmfDigitDetected;

    /* Sequencer state variables: */
    struct {

        /* If a sequencer-abort command has been sent to the SLAC, but the
           VP792_EVID_SEQ event has not yet arrived, this is TRUE. */
        bool aborting;

        /* Event handlers which will be started only after the current
           sequence is aborted: */
        uint16 deferredHandlers;

        /* Keeps track of what, if anything, is using the sequencer. */
        Vp792SequencerUseType activeSequence;

        /* Buffer for holding a new sequencer program while we wait for the
           old sequence to be aborted: */
#define _VP792_CMDSIZE_WR_SEQ_CTL 35
        uint16 program[_VP792_CMDSIZE_WR_SEQ_CTL];

        /* Register values are cached here during hook mask and RX-disable
           intervals: */
        uint16 lastDsh;
        uint16 lastVpCfg2;

        bool hookMaskInterval;
    } sequencer;

    /* State variables for VpSendSignal(): */
    struct {

        /* Stores the ongoing send signal type (if any), and the one that will
           be applied when the sequencer becomes available. */
        VpSendSignalType currentSignal;
        VpSendSignalType newSignal;

        /* Stores the value that was in the DRIVE_ST variable before the signal
           started. */
        uint16 driveSt;

        /* State variables for Vp792SendSignal(VP_SENDSIG_MSG_WAIT_PULSE): */
        struct {
            int16 voltage;

            struct {
                struct {
                    uint16 v1;
                    uint16 vas;
                    uint16 ila;
                } on;
                struct {
                    uint16 v1;
                    uint16 vas;
                    uint16 ila;
                } off;

                uint16 vasOffset;
                uint16 rfd;
                uint16 rptc;
            } dcParams;

            bool readingDcParams;
            bool aborting;

            uint16 requestIdx;  /* for RD_DC_PARAMS request */
        } msgWait;
    } sendSig;

    /* Caller ID state variables */
    struct {

        /* Currently active Caller ID profile.  This will be the same as
           profiles.pCidProfile if VpInitRing() was called, but it will be
           different if VpSendCid() was called. */
#ifdef VP_COMMON_ADDRESS_SPACE
        VpProfilePtrType pCidProfile;
#else
        uint16 cidProfileValid;
        VpProfileDataType cidProfile[VP792_CID_PROFILE_LEN];
#endif

        /* Sending data using FSK or DTMF? */
        bool isFsk;

        /* FSK data state */
        struct {
            /* Whether the API should compute the checksum */
            bool autoChecksum;

            /* SLAC device buffer status bits */
            uint8 bufStatus;

            /* Preamble */
            uint16 seizureBits;
            uint16 markBits;

            uint8 checksum;
        } fsk;

        struct {
            /* Message data buffer */
            bool nonempty;
            uint8 buf[VP_SIZEOF_CID_MSG_BUFFER * 2];
            uint8 head;
            uint8 tail;
        } data;

        bool dtmfDetectInterval;
        uint8 dtmfTones[2];
    } callerId;

    /* Metering state variables */
    struct {
        bool active;
        uint16 eventRate;
    } metering;
} Vp792LineObjectType;

/* Vp792 Group Device Object */

typedef struct
{

  VpDeviceIdType deviceId;

  /* Temporary storage for interrupt indication, while mailbox is busy. */
  uint16 intInd[2];

} Vp792GroupDeviceObjectType;

#endif /* VP792_API_H */
