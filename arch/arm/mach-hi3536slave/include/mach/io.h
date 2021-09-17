#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#define IO_SPACE_LIMIT  0xffffffff

#define __io(a)         __typesafe_io(a)
#define __mem_pci(a)    (a)

/*  phys_addr		virt_addr
 * 0x1000_0000 <-----> 0xFB00_0000
 */
#define HI3536_IOCH1_VIRT (0xFB000000)
#define HI3536_IOCH1_PHYS (0x10000000)
#define HI3536_IOCH1_SIZE (0x03190000)

/*  phys_addr		virt_addr
 * 0x1FFF0000 <-----> 0xFE400000
 * Only for s5 platform
 */
#define HI3536_IOCH2_VIRT (0xFE400000)
#define HI3536_IOCH2_PHYS (0x1FFF0000)
#define HI3536_IOCH2_SIZE (0x00010000)

#define IO_OFFSET_LOW  (0xEB000000)
#define IO_OFFSET_HIGH (0xDE410000)

#define IO_ADDRESS(x)   ((x) >= HI3536_IOCH2_PHYS ? (x) + IO_OFFSET_HIGH\
		: (x) + IO_OFFSET_LOW)

#define IO_ADDRESS_LOW(x)  ((x) + IO_OFFSET_LOW)
#define IO_ADDRESS_HIGH(x) ((x) + IO_OFFSET_HIGH)

#endif
