#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _ASM_PGTABLE_3LEVEL_TYPES_H
#define _ASM_PGTABLE_3LEVEL_TYPES_H

#include <asm/types.h>

typedef u64 pteval_t;
typedef u64 pmdval_t;
typedef u64 pgdval_t;

#undef STRICT_MM_TYPECHECKS

#ifdef STRICT_MM_TYPECHECKS


#ifdef CONFIG_SYNO_ALPINE
typedef struct { pteval_t pte;
#if HW_PAGES_PER_PAGE > 1
	pteval_t unused[HW_PAGES_PER_PAGE-1];
#endif  
} pte_t;
#else
typedef struct { pteval_t pte; } pte_t;
#endif
typedef struct { pmdval_t pmd; } pmd_t;
typedef struct { pgdval_t pgd; } pgd_t;
typedef struct { pteval_t pgprot; } pgprot_t;

#define pte_val(x)      ((x).pte)
#define pmd_val(x)      ((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)   ((x).pgprot)

#ifdef CONFIG_SYNO_ALPINE
#define __pte(x)        ({pte_t __pte = { .pte = (x) }; __pte; })
#else
#define __pte(x)        ((pte_t) { (x) } )
#endif
#define __pmd(x)        ((pmd_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)     ((pgprot_t) { (x) } )

#else	 

#ifdef CONFIG_SYNO_ALPINE
typedef struct { pteval_t pte;
#if HW_PAGES_PER_PAGE > 1
	pteval_t unused[HW_PAGES_PER_PAGE-1];
#endif  
} pte_t;
#else
typedef pteval_t pte_t;
#endif
typedef pmdval_t pmd_t;
typedef pgdval_t pgd_t;
typedef pteval_t pgprot_t;

#ifdef CONFIG_SYNO_ALPINE
#define pte_val(x)      ((x).pte)
#else
#define pte_val(x)	(x)
#endif
#define pmd_val(x)	(x)
#define pgd_val(x)	(x)
#define pgprot_val(x)	(x)

#ifdef CONFIG_SYNO_ALPINE
#define __pte(x)        ({pte_t __pte = { .pte = (x) }; __pte; })
#else
#define __pte(x)	(x)
#endif
#define __pmd(x)	(x)
#define __pgd(x)	(x)
#define __pgprot(x)	(x)

#endif	 

#endif	 
