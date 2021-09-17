#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __MV_SYSMAP_PCI_H
#define __MV_SYSMAP_PCI_H

#define DFXW				1
#define SCPUW				2
#define DITCMW				3
#define DDTCMW				4

#define DRAGONITE_DTCM_OFFSET		0x04000000

#define DFX_SIZE			_1M
#if defined(MY_ABC_HERE)
#define SCPU_SIZE			_2M
#else  
#define SCPU_SIZE			_512K
#endif  
#define ITCM_SIZE			_64K
#define DTCM_SIZE			_64K

#define DFX_BASE			0x0
#if defined(MY_ABC_HERE)
#define SCPU_BASE			(DFX_BASE + DFX_SIZE + _1M) 
#else  
#define SCPU_BASE			(DFX_BASE + DFX_SIZE)
#endif  
#define ITCM_BASE			(SCPU_BASE + SCPU_SIZE)
#define DTCM_BASE			(ITCM_BASE + ITCM_SIZE)

#endif  
