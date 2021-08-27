#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _ASM_X86_SERIAL_H
#define _ASM_X86_SERIAL_H

#ifdef CONFIG_GEN3_UART
 
#define BASE_BAUD ( 14745600 / 16 )
#else
#define BASE_BAUD ( 1843200 / 16 )
#endif

#ifdef CONFIG_SERIAL_DETECT_IRQ
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST | ASYNC_AUTO_IRQ)
#define STD_COM4_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_AUTO_IRQ)
#else
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)
#define STD_COM4_FLAGS ASYNC_BOOT_AUTOCONF
#endif

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)

#ifdef CONFIG_GEN3_UART
 
#ifdef CONFIG_SMP
#define SERIAL_PORT_DFNS            \
               \
    { 0, BASE_BAUD, 0x3F8, 38, STD_COM_FLAGS },   \
    { 0, BASE_BAUD, 0x2F8, 38, STD_COM_FLAGS },   \
    { 0, BASE_BAUD, 0x3E8, 38, STD_COM_FLAGS },  
#else
#define SERIAL_PORT_DFNS            \
               \
    { 0, BASE_BAUD, 0x3F8, 4, STD_COM_FLAGS },    \
    { 0, BASE_BAUD, 0x2F8, 4, STD_COM_FLAGS },    \
    { 0, BASE_BAUD, 0x3E8, 4, STD_COM_FLAGS },   
#endif
#else
#if !defined(CONFIG_SYNO_CEDARVIEW) && !defined(CONFIG_SYNO_AVOTON)
#define SERIAL_PORT_DFNS            \
	           \
	{ 0, BASE_BAUD, 0x2F8, 3, STD_COM_FLAGS },    \
	{ 0, BASE_BAUD, 0x3F8, 4, STD_COM_FLAGS },    \
	{ 0, BASE_BAUD, 0x3E8, 4, STD_COM_FLAGS },    \
	{ 0, BASE_BAUD, 0x2E8, 3, STD_COM4_FLAGS },  
#else
#define SERIAL_PORT_DFNS			\
	 			\
	{ 0, BASE_BAUD, 0x3F8, 4, STD_COM_FLAGS },	 	\
	{ 0, BASE_BAUD, 0x2F8, 3, STD_COM_FLAGS },	 	\
	{ 0, BASE_BAUD, 0x3E8, 4, STD_COM_FLAGS },	 	\
	{ 0, BASE_BAUD, 0x2E8, 3, STD_COM4_FLAGS },	 
#endif  

#endif  
#else
#define SERIAL_PORT_DFNS			\
	 			\
	{ 0, BASE_BAUD, 0x3F8, 4, STD_COM_FLAGS },	 	\
	{ 0, BASE_BAUD, 0x2F8, 3, STD_COM_FLAGS },	 	\
	{ 0, BASE_BAUD, 0x3E8, 4, STD_COM_FLAGS },	 	\
	{ 0, BASE_BAUD, 0x2E8, 3, STD_COM4_FLAGS },	 
#endif  

#endif  
