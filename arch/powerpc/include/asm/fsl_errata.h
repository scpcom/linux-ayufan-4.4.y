 
#ifdef __KERNEL__
#ifndef __ASM_FSL_ERRATA_H__
#define __ASM_FSL_ERRATA_H__

#ifdef CONFIG_SYNO_MPC85XX_COMMON

#define SVR_MAJ(svr)    (((svr) >>  4) & 0xF)    
#define SVR_MIN(svr)    (((svr) >>  0) & 0xF)    

#define MPC8548_ERRATA(maj, min) \
       ((0x80210000 == (0xFFFF0000 & mfspr(SPRN_PVR))) && \
	(SVR_MAJ(mfspr(SPRN_SVR)) <= maj) && (SVR_MIN(mfspr(SPRN_SVR)) <= min))
#else

#define MPC8548_ERRATA(maj, min) (0)

#endif

#endif
#endif
