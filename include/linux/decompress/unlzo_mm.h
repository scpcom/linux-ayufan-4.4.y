#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef UNLZO_MM_H
#define UNLZO_MM_H

#if defined(CONFIG_SYNO_COMCERTO)
#ifdef STATIC
#define INIT
#else
#define INIT __init
#endif
#endif

#endif
