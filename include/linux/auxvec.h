#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_AUXVEC_H
#define _LINUX_AUXVEC_H

#include <uapi/linux/auxvec.h>

#if defined(MY_DEF_HERE)
// adjust size of struct mm_struct
#define AT_VECTOR_SIZE_BASE 19
#else /* MY_DEF_HERE */
#define AT_VECTOR_SIZE_BASE 20 /* NEW_AUX_ENT entries in auxiliary table */
  /* number of "#define AT_.*" above, minus {AT_NULL, AT_IGNORE, AT_NOTELF} */
#endif /* MY_DEF_HERE */
#endif /* _LINUX_AUXVEC_H */
