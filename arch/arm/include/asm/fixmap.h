#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _ASM_FIXMAP_H
#define _ASM_FIXMAP_H

#if (defined(MY_DEF_HERE) && defined(CONFIG_MV_SUPPORT_64KB_PAGE_SIZE) && defined(CONFIG_HIGHMEM)) || \
     (defined(MY_DEF_HERE) && defined(CONFIG_MV_LARGE_PAGE_SUPPORT) && defined(CONFIG_HIGHMEM)) 
#define FIXADDR_START		0xffc00000UL
#define FIXADDR_TOP		0xfff00000UL
#elif defined(MY_DEF_HERE) && defined(CONFIG_ARM_PAGE_SIZE_LARGE) && defined(CONFIG_HIGHMEM)
#define FIXADDR_START          0xff200000UL
#define FIXADDR_TOP            0xff600000UL
#else
#if defined(MY_ABC_HERE)
#define FIXADDR_START		0xfff30000UL
#else
#define FIXADDR_START		0xfff00000UL
#endif
#define FIXADDR_TOP		0xfffe0000UL
#endif
#define FIXADDR_SIZE		(FIXADDR_TOP - FIXADDR_START)

#define FIX_KMAP_BEGIN		0
#define FIX_KMAP_END		(FIXADDR_SIZE >> PAGE_SHIFT)

#define __fix_to_virt(x)	(FIXADDR_START + ((x) << PAGE_SHIFT))
#define __virt_to_fix(x)	(((x) - FIXADDR_START) >> PAGE_SHIFT)

extern void __this_fixmap_does_not_exist(void);

static inline unsigned long fix_to_virt(const unsigned int idx)
{
	if (idx >= FIX_KMAP_END)
		__this_fixmap_does_not_exist();
	return __fix_to_virt(idx);
}

static inline unsigned int virt_to_fix(const unsigned long vaddr)
{
	BUG_ON(vaddr >= FIXADDR_TOP || vaddr < FIXADDR_START);
	return __virt_to_fix(vaddr);
}

#endif
