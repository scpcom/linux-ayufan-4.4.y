 /* clflushopt falls back to clflush
  * if clflushopt is not available */
 #define clflushopt clflush

/* from asm-generic/barrier.h */
#ifndef smp_mb__before_atomic
#define smp_mb__before_atomic()        smp_mb()
#endif

#ifndef smp_mb__after_atomic
#define smp_mb__after_atomic() smp_mb()
#endif

/* from include/acpi/acpi_io.h */
/* We apparently don't want to include linux/acpi_io.h directly as that
 * can cause build problems in our kernel */
#include <linux/acpi.h>
#include <linux/io.h>

static inline void __iomem *acpi_os_ioremap(acpi_physical_address phys,
                                           acpi_size size)
{
       return ioremap_cache(phys, size);
}

#undef dma_buf_export
#define dma_buf_export(priv, ops, size, flags, resv)	\
	dma_buf_export_named(priv, ops, size, flags, KBUILD_MODNAME)
