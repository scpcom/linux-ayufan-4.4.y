#include <linux/linkage.h>

ENTRY(ls1024a_secondary_startup)
	bl	v7_invalidate_l1
	b	secondary_startup
ENDPROC(ls1024a_secondary_startup)
