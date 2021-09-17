#ifndef _LINUX_AUXVEC_H
#define _LINUX_AUXVEC_H

#include <uapi/linux/auxvec.h>

#if defined(CONFIG_SYNO_HI3536_ALIGN_STRUCTURES)
// adjust size of struct mm_struct
#define AT_VECTOR_SIZE_BASE 19
#else /* CONFIG_SYNO_HI3536_ALIGN_STRUCTURES */
#define AT_VECTOR_SIZE_BASE 20 /* NEW_AUX_ENT entries in auxiliary table */
  /* number of "#define AT_.*" above, minus {AT_NULL, AT_IGNORE, AT_NOTELF} */
#endif /* CONFIG_SYNO_HI3536_ALIGN_STRUCTURES */
#endif /* _LINUX_AUXVEC_H */
