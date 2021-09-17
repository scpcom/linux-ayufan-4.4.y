
#ifndef __HIOS_ARM_TYPE_H__
#define __HIOS_ARM_TYPE_H__

typedef unsigned char	hios_u8;
typedef unsigned short	hios_u16;
typedef unsigned long	hios_u32;

typedef signed char	hios_s8;
typedef signed short	hios_s16;
typedef signed long	hios_s32;

#if defined(__GNUC__)
typedef signed long long	hios_s64;
typedef unsigned long long	hios_u64;
#endif

typedef char		hios_char;
typedef void		hios_void;
typedef unsigned int	hios_size;
typedef unsigned int	hios_virt_addr;
typedef unsigned int	hios_phy_addr;
typedef unsigned int	hios_dma_addr;

typedef int		hios_stat;
typedef int		hios_int;
typedef int		hios_pid;
typedef int		hios_status;

#define hios_const          const
#define HIOS_NULL           NULL

#endif /* __HIOS_ARM_TYPE_H__ */
