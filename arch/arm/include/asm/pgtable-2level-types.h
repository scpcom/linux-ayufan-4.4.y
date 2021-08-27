#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _ASM_PGTABLE_2LEVEL_TYPES_H
#define _ASM_PGTABLE_2LEVEL_TYPES_H

#include <asm/types.h>

typedef u32 pteval_t;
typedef u32 pmdval_t;

#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_64K_PAGES)
#undef STRICT_MM_TYPECHECKS
#else
#define STRICT_MM_TYPECHECKS	1
#endif

#ifdef STRICT_MM_TYPECHECKS
 
#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_64K_PAGES)
typedef struct { pteval_t pte; } pte_t;
typedef struct { pmdval_t pmd; } pmd_t;
typedef struct { pmdval_t pgd[2]; } pgd_t;
typedef struct { pteval_t pgprot; } pgprot_t;

#define pte_val(x)      ((x).pte)
#define pmd_val(x)      ((x).pmd)
#define pgd_val(x)	((x).pgd[0])
#define pgprot_val(x)   ((x).pgprot)

#define __pte(x)        ((pte_t) { (x) } )
#define __pmd(x)        ((pmd_t) { (x) } )
#define __pgprot(x)     ((pgprot_t) { (x) } )

#else
#include <asm/pgtable-2level.h>
typedef struct { pteval_t pte[16]; } pte_t;
typedef struct { pmdval_t pmd; } pmd_t;
typedef struct { pmdval_t pgd[LINKED_PMDS]; } pgd_t;
typedef struct { pteval_t pgprot; } pgprot_t;

#define pte_val(x)      ((x).pte[0])
#define pmd_val(x)      ((x).pmd)
#define pgd_val(x)	((x).pgd[0])
#define pgprot_val(x)   ((x).pgprot)

#define __pte(x)        ((pte_t) { {(x)} } )
#define __pmd(x)        ((pmd_t) { (x) } )
#define __pgprot(x)     ((pgprot_t) { (x) } )
#endif

#else
 
typedef pteval_t pte_t;
typedef pmdval_t pmd_t;
typedef pmdval_t pgd_t[2];
typedef pteval_t pgprot_t;

#define pte_val(x)      (x)
#define pmd_val(x)      (x)
#define pgd_val(x)	((x)[0])
#define pgprot_val(x)   (x)

#define __pte(x)        (x)
#define __pmd(x)        (x)
#define __pgprot(x)     (x)

#endif  

#endif	 
