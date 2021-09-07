#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/screen_info.h>
#include <linux/ioport.h>
#include <linux/acpi.h>
#include <linux/sfi.h>
#include <linux/apm_bios.h>
#include <linux/initrd.h>
#include <linux/bootmem.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/mca.h>
#include <linux/root_dev.h>
#include <linux/highmem.h>
#include <linux/module.h>
#include <linux/efi.h>
#include <linux/init.h>
#include <linux/edd.h>
#include <linux/iscsi_ibft.h>
#include <linux/nodemask.h>
#include <linux/kexec.h>
#include <linux/dmi.h>
#include <linux/pfn.h>
#include <linux/pci.h>
#include <asm/pci-direct.h>
#include <linux/init_ohci1394_dma.h>
#include <linux/kvm_para.h>

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/user.h>
#include <linux/delay.h>

#include <linux/kallsyms.h>
#include <linux/cpufreq.h>
#include <linux/dma-mapping.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>

#include <linux/percpu.h>
#include <linux/crash_dump.h>
#include <linux/tboot.h>

#include <video/edid.h>

#include <asm/mtrr.h>
#include <asm/apic.h>
#include <asm/e820.h>
#include <asm/mpspec.h>
#include <asm/setup.h>
#include <asm/efi.h>
#include <asm/timer.h>
#include <asm/i8259.h>
#include <asm/sections.h>
#include <asm/dmi.h>
#include <asm/io_apic.h>
#include <asm/ist.h>
#include <asm/vmi.h>
#include <asm/setup_arch.h>
#include <asm/bios_ebda.h>
#include <asm/cacheflush.h>
#include <asm/processor.h>
#include <asm/bugs.h>

#include <asm/system.h>
#include <asm/vsyscall.h>
#include <asm/cpu.h>
#include <asm/desc.h>
#include <asm/dma.h>
#include <asm/iommu.h>
#include <asm/gart.h>
#include <asm/mmu_context.h>
#include <asm/proto.h>

#include <asm/paravirt.h>
#include <asm/hypervisor.h>

#include <asm/percpu.h>
#include <asm/topology.h>
#include <asm/apicdef.h>
#ifdef CONFIG_X86_64
#include <asm/numa_64.h>
#endif
#include <asm/mce.h>

#ifdef  MY_ABC_HERE
#include  <linux/synobios.h>
#endif

#ifdef MY_ABC_HERE
extern long g_internal_hd_num;
#endif

#ifdef MY_ABC_HERE
extern long g_internal_netif_num;
#endif

#ifdef SYNO_SAS_DISK_NAME
extern long g_is_sas_model;
#endif

#ifdef MY_ABC_HERE
extern long g_ahci_switch;
#endif

#ifdef MY_ABC_HERE
extern long g_sata_led_special;
#endif

#ifdef MY_ABC_HERE
extern long g_hdd_hotplug;
#endif

#ifdef MY_ABC_HERE
extern char gszSataPortMap[8];
#endif

#ifdef MY_ABC_HERE
extern char gszDiskIdxMap[16];
#endif

#ifdef MY_ABC_HERE
extern char giDiskSeqReverse[8];
#endif

#ifdef MY_ABC_HERE
extern unsigned char grgbLanMac[4][16];
#endif

#ifdef MY_ABC_HERE
extern char gszSerialNum[32];
extern char gszCustomSerialNum[32];
#endif

#ifdef MY_ABC_HERE
extern unsigned int gSwitchDev;
extern char gDevPCIName[SYNO_MAX_SWITCHABLE_NET_DEVICE][SYNO_NET_DEVICE_ENCODING_LENGTH];
#endif

unsigned long max_low_pfn_mapped;
unsigned long max_pfn_mapped;

RESERVE_BRK(dmi_alloc, 65536);

unsigned int boot_cpu_id __read_mostly;

static __initdata unsigned long _brk_start = (unsigned long)__brk_base;
unsigned long _brk_end = (unsigned long)__brk_base;

#ifdef CONFIG_X86_64
int default_cpu_present_to_apicid(int mps_cpu)
{
	return __default_cpu_present_to_apicid(mps_cpu);
}

int default_check_phys_apicid_present(int phys_apicid)
{
	return __default_check_phys_apicid_present(phys_apicid);
}
#endif

#ifndef CONFIG_DEBUG_BOOT_PARAMS
struct boot_params __initdata boot_params;
#else
struct boot_params boot_params;
#endif

static struct resource data_resource = {
	.name	= "Kernel data",
	.start	= 0,
	.end	= 0,
	.flags	= IORESOURCE_BUSY | IORESOURCE_MEM
};

static struct resource code_resource = {
	.name	= "Kernel code",
	.start	= 0,
	.end	= 0,
	.flags	= IORESOURCE_BUSY | IORESOURCE_MEM
};

static struct resource bss_resource = {
	.name	= "Kernel bss",
	.start	= 0,
	.end	= 0,
	.flags	= IORESOURCE_BUSY | IORESOURCE_MEM
};

#ifdef CONFIG_X86_32
 
struct cpuinfo_x86 new_cpu_data __cpuinitdata = {0, 0, 0, 0, -1, 1, 0, 0, -1};
 
struct cpuinfo_x86 boot_cpu_data __read_mostly = {0, 0, 0, 0, -1, 1, 0, 0, -1};
EXPORT_SYMBOL(boot_cpu_data);
static void set_mca_bus(int x)
{
#ifdef CONFIG_MCA
	MCA_bus = x;
#endif
}

unsigned int def_to_bigsmp;

unsigned int machine_id;
unsigned int machine_submodel_id;
unsigned int BIOS_revision;

struct apm_info apm_info;
EXPORT_SYMBOL(apm_info);

#if defined(CONFIG_X86_SPEEDSTEP_SMI) || \
	defined(CONFIG_X86_SPEEDSTEP_SMI_MODULE)
struct ist_info ist_info;
EXPORT_SYMBOL(ist_info);
#else
struct ist_info ist_info;
#endif

#else
struct cpuinfo_x86 boot_cpu_data __read_mostly = {
	.x86_phys_bits = MAX_PHYSMEM_BITS,
};
EXPORT_SYMBOL(boot_cpu_data);
#endif

#if !defined(CONFIG_X86_PAE) || defined(CONFIG_X86_64)
unsigned long mmu_cr4_features;
#else
unsigned long mmu_cr4_features = X86_CR4_PAE;
#endif

int bootloader_type, bootloader_version;

struct screen_info screen_info;
EXPORT_SYMBOL(screen_info);
struct edid_info edid_info;
EXPORT_SYMBOL_GPL(edid_info);

extern int root_mountflags;

unsigned long saved_video_mode;

#define RAMDISK_IMAGE_START_MASK	0x07FF
#define RAMDISK_PROMPT_FLAG		0x8000
#define RAMDISK_LOAD_FLAG		0x4000

static char __initdata command_line[COMMAND_LINE_SIZE];
#ifdef CONFIG_CMDLINE_BOOL
static char __initdata builtin_cmdline[COMMAND_LINE_SIZE] = CONFIG_CMDLINE;
#endif

#if defined(CONFIG_EDD) || defined(CONFIG_EDD_MODULE)
struct edd edd;
#ifdef CONFIG_EDD_MODULE
EXPORT_SYMBOL(edd);
#endif
 
static inline void copy_edd(void)
{
     memcpy(edd.mbr_signature, boot_params.edd_mbr_sig_buffer,
	    sizeof(edd.mbr_signature));
     memcpy(edd.edd_info, boot_params.eddbuf, sizeof(edd.edd_info));
     edd.mbr_signature_nr = boot_params.edd_mbr_sig_buf_entries;
     edd.edd_info_nr = boot_params.eddbuf_entries;
}
#else
static inline void copy_edd(void)
{
}
#endif

void * __init extend_brk(size_t size, size_t align)
{
	size_t mask = align - 1;
	void *ret;

	BUG_ON(_brk_start == 0);
	BUG_ON(align & mask);

	_brk_end = (_brk_end + mask) & ~mask;
	BUG_ON((char *)(_brk_end + size) > __brk_limit);

	ret = (void *)_brk_end;
	_brk_end += size;

	memset(ret, 0, size);

	return ret;
}

#ifdef CONFIG_X86_64
static void __init init_gbpages(void)
{
	if (direct_gbpages && cpu_has_gbpages)
		printk(KERN_INFO "Using GB pages for direct mapping\n");
	else
		direct_gbpages = 0;
}
#else
static inline void init_gbpages(void)
{
}
#endif

static void __init reserve_brk(void)
{
	if (_brk_end > _brk_start)
		reserve_early(__pa(_brk_start), __pa(_brk_end), "BRK");

	_brk_start = 0;
}

#ifdef CONFIG_BLK_DEV_INITRD

#define MAX_MAP_CHUNK	(NR_FIX_BTMAPS << PAGE_SHIFT)
static void __init relocate_initrd(void)
{

	u64 ramdisk_image = boot_params.hdr.ramdisk_image;
	u64 ramdisk_size  = boot_params.hdr.ramdisk_size;
	u64 end_of_lowmem = max_low_pfn_mapped << PAGE_SHIFT;
	u64 ramdisk_here;
	unsigned long slop, clen, mapaddr;
	char *p, *q;

	ramdisk_here = find_e820_area(0, end_of_lowmem, ramdisk_size,
					 PAGE_SIZE);

	if (ramdisk_here == -1ULL)
		panic("Cannot find place for new RAMDISK of size %lld\n",
			 ramdisk_size);

	reserve_early(ramdisk_here, ramdisk_here + ramdisk_size,
			 "NEW RAMDISK");
	initrd_start = ramdisk_here + PAGE_OFFSET;
	initrd_end   = initrd_start + ramdisk_size;
	printk(KERN_INFO "Allocated new RAMDISK: %08llx - %08llx\n",
			 ramdisk_here, ramdisk_here + ramdisk_size);

	q = (char *)initrd_start;

	if (ramdisk_image < end_of_lowmem) {
		clen = end_of_lowmem - ramdisk_image;
		p = (char *)__va(ramdisk_image);
		memcpy(q, p, clen);
		q += clen;
		ramdisk_image += clen;
		ramdisk_size  -= clen;
	}

	while (ramdisk_size) {
		slop = ramdisk_image & ~PAGE_MASK;
		clen = ramdisk_size;
		if (clen > MAX_MAP_CHUNK-slop)
			clen = MAX_MAP_CHUNK-slop;
		mapaddr = ramdisk_image & PAGE_MASK;
		p = early_memremap(mapaddr, clen+slop);
		memcpy(q, p+slop, clen);
		early_iounmap(p, clen+slop);
		q += clen;
		ramdisk_image += clen;
		ramdisk_size  -= clen;
	}
	 
	ramdisk_image = boot_params.hdr.ramdisk_image;
	ramdisk_size  = boot_params.hdr.ramdisk_size;
	printk(KERN_INFO "Move RAMDISK from %016llx - %016llx to"
		" %08llx - %08llx\n",
		ramdisk_image, ramdisk_image + ramdisk_size - 1,
		ramdisk_here, ramdisk_here + ramdisk_size - 1);
}

static void __init reserve_initrd(void)
{
	u64 ramdisk_image = boot_params.hdr.ramdisk_image;
	u64 ramdisk_size  = boot_params.hdr.ramdisk_size;
	u64 ramdisk_end   = ramdisk_image + ramdisk_size;
	u64 end_of_lowmem = max_low_pfn_mapped << PAGE_SHIFT;

	if (!boot_params.hdr.type_of_loader ||
	    !ramdisk_image || !ramdisk_size)
		return;		 

	initrd_start = 0;

	if (ramdisk_size >= (end_of_lowmem>>1)) {
		free_early(ramdisk_image, ramdisk_end);
		printk(KERN_ERR "initrd too large to handle, "
		       "disabling initrd\n");
		return;
	}

	printk(KERN_INFO "RAMDISK: %08llx - %08llx\n", ramdisk_image,
			ramdisk_end);

	if (ramdisk_end <= end_of_lowmem) {
		 
		initrd_start = ramdisk_image + PAGE_OFFSET;
		initrd_end = initrd_start + ramdisk_size;
		return;
	}

	relocate_initrd();

	free_early(ramdisk_image, ramdisk_end);
}
#else
static void __init reserve_initrd(void)
{
}
#endif  

static void __init parse_setup_data(void)
{
	struct setup_data *data;
	u64 pa_data;

	if (boot_params.hdr.version < 0x0209)
		return;
	pa_data = boot_params.hdr.setup_data;
	while (pa_data) {
		data = early_memremap(pa_data, PAGE_SIZE);
		switch (data->type) {
		case SETUP_E820_EXT:
			parse_e820_ext(data, pa_data);
			break;
		default:
			break;
		}
		pa_data = data->next;
		early_iounmap(data, PAGE_SIZE);
	}
}

static void __init e820_reserve_setup_data(void)
{
	struct setup_data *data;
	u64 pa_data;
	int found = 0;

	if (boot_params.hdr.version < 0x0209)
		return;
	pa_data = boot_params.hdr.setup_data;
	while (pa_data) {
		data = early_memremap(pa_data, sizeof(*data));
		e820_update_range(pa_data, sizeof(*data)+data->len,
			 E820_RAM, E820_RESERVED_KERN);
		found = 1;
		pa_data = data->next;
		early_iounmap(data, sizeof(*data));
	}
	if (!found)
		return;

	sanitize_e820_map(e820.map, ARRAY_SIZE(e820.map), &e820.nr_map);
	memcpy(&e820_saved, &e820, sizeof(struct e820map));
	printk(KERN_INFO "extended physical RAM map:\n");
	e820_print_map("reserve setup_data");
}

static void __init reserve_early_setup_data(void)
{
	struct setup_data *data;
	u64 pa_data;
	char buf[32];

	if (boot_params.hdr.version < 0x0209)
		return;
	pa_data = boot_params.hdr.setup_data;
	while (pa_data) {
		data = early_memremap(pa_data, sizeof(*data));
		sprintf(buf, "setup data %x", data->type);
		reserve_early(pa_data, pa_data+sizeof(*data)+data->len, buf);
		pa_data = data->next;
		early_iounmap(data, sizeof(*data));
	}
}

#ifdef MY_ABC_HERE
static int __init early_hw_version(char *p)
{
	char *szPtr;

	snprintf(gszSynoHWVersion, 16, "%s", p);

	szPtr = gszSynoHWVersion;
	while ((*szPtr != ' ') && (*szPtr != '\t') && (*szPtr != '\0')) {
		szPtr++;
	}
	*szPtr = 0;
	strcat(gszSynoHWVersion, "-j");

	printk("Synology Hardware Version: %s\n", gszSynoHWVersion);

	return 1;
}
__setup("syno_hw_version=", early_hw_version);
#endif

#ifdef MY_ABC_HERE
static int __init early_internal_hd_num(char *p)
{
	g_internal_hd_num = simple_strtol(p, NULL, 10);

	printk("Internal HD num: %d\n", (int)g_internal_hd_num);

    return 1;
}
__setup("ihd_num=", early_internal_hd_num);
#endif

#ifdef  MY_ABC_HERE
static int __init early_internal_netif_num(char *p)
{
	g_internal_netif_num = simple_strtol(p, NULL, 10);

	if ( g_internal_netif_num >= 0 ) {
		printk("Internal netif num: %d\n", (int)g_internal_netif_num);
	}

	return 1;
}
__setup("netif_num=", early_internal_netif_num);
#endif

#ifdef SYNO_SAS_DISK_NAME
static int __init early_SASmodel(char *p)
{
	g_is_sas_model = simple_strtol(p, NULL, 10);

	if ( 1 == g_is_sas_model) {
		printk("SAS model: %d\n", (int)g_is_sas_model);
	}

	return 1;
}
__setup("SASmodel=", early_SASmodel);
#endif

#ifdef  MY_ABC_HERE
static int __init early_ahci_switch(char *p)
{
        g_ahci_switch = simple_strtol(p, NULL, 10);

        if ( g_ahci_switch >= 0 ) {
                printk("AHCI: %d\n", (int)g_ahci_switch);
        }

        return 1;
}
__setup("ahci=", early_ahci_switch);
#endif

#ifdef MY_ABC_HERE
static int __init early_hdd_hotplug(char *p)
{
	g_hdd_hotplug = simple_strtol(p, NULL, 10);

	if ( g_hdd_hotplug > 0 ) {
		printk("Support HDD Hotplug.\n");
	}

	return 1;
}
__setup("HddHotplug=", early_hdd_hotplug);
#endif

#ifdef MY_ABC_HERE
static int __init early_sataled_special(char *p)
{
	g_sata_led_special = simple_strtol(p, NULL, 10);

	if ( g_sata_led_special >= 0 ) {
		printk("Special Sata LEDs.\n");
	}

	return 1;
}
__setup("SataLedSpecial=", early_sataled_special);
#endif

#ifdef MY_ABC_HERE
static int __init early_sataport_map(char *p)
{
	snprintf(gszSataPortMap, sizeof(gszSataPortMap), "%s", p);

	if(0 != gszSataPortMap[0]) {
		printk("Sata Port Map: %s\n", gszSataPortMap);
	}

	return 1;
}
__setup("SataPortMap=", early_sataport_map);
#endif

#ifdef MY_ABC_HERE
static int __init early_disk_idx_map(char *p)
{
	snprintf(gszDiskIdxMap, sizeof(gszDiskIdxMap), "%s", p);

	if('\0' != gszDiskIdxMap[0]) {
		printk("Disk Index Map: %s\n", gszDiskIdxMap);
	}

	return 1;
}
__setup("DiskIdxMap=", early_disk_idx_map);
#endif

#ifdef MY_ABC_HERE
static int __init early_disk_seq_reserve(char *p)
{
	snprintf(giDiskSeqReverse, sizeof(giDiskSeqReverse), "%s", p);

	if('\0' != giDiskSeqReverse[0]) {
		printk("Disk Sequence Reverse: %s\n", giDiskSeqReverse);
	}

	return 1;
}
__setup("DiskSeqReverse=", early_disk_seq_reserve);
#endif

#ifdef MY_ABC_HERE
static int __init early_mac1(char *p)
{
	snprintf(grgbLanMac[0], sizeof(grgbLanMac[0]), "%s", p);

	printk("Mac1: %s\n", grgbLanMac[0]);

	return 1;
}
__setup("mac1=", early_mac1);

static int __init early_mac2(char *p)
{
	snprintf(grgbLanMac[1], sizeof(grgbLanMac[1]), "%s", p);

	printk("Mac2: %s\n", grgbLanMac[1]);

	return 1;
}
__setup("mac2=", early_mac2);

static int __init early_mac3(char *p)
{
	snprintf(grgbLanMac[2], sizeof(grgbLanMac[2]), "%s", p);

	printk("Mac3: %s\n", grgbLanMac[2]);

	return 1;
}
__setup("mac3=", early_mac3);

static int __init early_mac4(char *p)
{
	snprintf(grgbLanMac[3], sizeof(grgbLanMac[3]), "%s", p);

	printk("Mac4: %s\n", grgbLanMac[3]);

	return 1;
}
__setup("mac4=", early_mac4);
#endif

#ifdef MY_ABC_HERE
static int __init early_netif_seq(char *p)
{
	int len;
	int netDevCount;

	if ((NULL == p) || (0 == (len = strlen(p)))) {
		return 1;
	}

	if (len <= SYNO_MAX_SWITCHABLE_NET_DEVICE) {
		netDevCount = len;
		for(gSwitchDev = 0 ; gSwitchDev < netDevCount && gSwitchDev < SYNO_MAX_SWITCHABLE_NET_DEVICE ; gSwitchDev++) {
			gDevPCIName[gSwitchDev][0] = *p++;
		}
		return 1;
	}

	netDevCount = len/SYNO_NET_DEVICE_ENCODING_LENGTH;
	if (0 == netDevCount) {
		return 1;
	}

	for(gSwitchDev = 0 ; gSwitchDev < netDevCount && gSwitchDev < SYNO_MAX_SWITCHABLE_NET_DEVICE ; gSwitchDev++) {
		 
		memcpy(gDevPCIName[gSwitchDev], p, SYNO_NET_DEVICE_ENCODING_LENGTH);
		p += SYNO_NET_DEVICE_ENCODING_LENGTH;
	}
	return 1;
}

__setup("netif_seq=",early_netif_seq);
#endif

#ifdef MY_ABC_HERE
static int __init early_sn(char *p)
{
        snprintf(gszSerialNum, sizeof(gszSerialNum), "%s", p);
        printk("Serial Number: %s\n", gszSerialNum);
        return 1;
}
__setup("sn=", early_sn);

static int __init early_custom_sn(char *p)
{
        snprintf(gszCustomSerialNum, sizeof(gszCustomSerialNum), "%s", p);
        printk("Custom Serial Number: %s\n", gszCustomSerialNum);
        return 1;
}
__setup("custom_sn=", early_custom_sn);
#endif

#ifdef CONFIG_KEXEC

static
unsigned long long __init find_and_reserve_crashkernel(unsigned long long size)
{
	const unsigned long long alignment = 16<<20; 	 
	unsigned long long start = 0LL;

	while (1) {
		int ret;

		start = find_e820_area(start, ULONG_MAX, size, alignment);
		if (start == -1ULL)
			return start;

		ret = reserve_bootmem_generic(start, size, BOOTMEM_EXCLUSIVE);
		if (ret >= 0)
			return start;

		start += alignment;
	}
}

static inline unsigned long long get_total_mem(void)
{
	unsigned long long total;

	total = max_low_pfn - min_low_pfn;
#ifdef CONFIG_HIGHMEM
	total += highend_pfn - highstart_pfn;
#endif

	return total << PAGE_SHIFT;
}

static void __init reserve_crashkernel(void)
{
	unsigned long long total_mem;
	unsigned long long crash_size, crash_base;
	int ret;

	total_mem = get_total_mem();

	ret = parse_crashkernel(boot_command_line, total_mem,
			&crash_size, &crash_base);
	if (ret != 0 || crash_size <= 0)
		return;

	if (crash_base <= 0) {
		crash_base = find_and_reserve_crashkernel(crash_size);
		if (crash_base == -1ULL) {
			pr_info("crashkernel reservation failed. "
				"No suitable area found.\n");
			return;
		}
	} else {
		ret = reserve_bootmem_generic(crash_base, crash_size,
					BOOTMEM_EXCLUSIVE);
		if (ret < 0) {
			pr_info("crashkernel reservation failed - "
				"memory is in use\n");
			return;
		}
	}

	printk(KERN_INFO "Reserving %ldMB of memory at %ldMB "
			"for crashkernel (System RAM: %ldMB)\n",
			(unsigned long)(crash_size >> 20),
			(unsigned long)(crash_base >> 20),
			(unsigned long)(total_mem >> 20));

	crashk_res.start = crash_base;
	crashk_res.end   = crash_base + crash_size - 1;
	insert_resource(&iomem_resource, &crashk_res);
}
#else
static void __init reserve_crashkernel(void)
{
}
#endif

static struct resource standard_io_resources[] = {
	{ .name = "dma1", .start = 0x00, .end = 0x1f,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "pic1", .start = 0x20, .end = 0x21,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "timer0", .start = 0x40, .end = 0x43,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "timer1", .start = 0x50, .end = 0x53,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "keyboard", .start = 0x60, .end = 0x60,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "keyboard", .start = 0x64, .end = 0x64,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "dma page reg", .start = 0x80, .end = 0x8f,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "pic2", .start = 0xa0, .end = 0xa1,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "dma2", .start = 0xc0, .end = 0xdf,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO },
	{ .name = "fpu", .start = 0xf0, .end = 0xff,
		.flags = IORESOURCE_BUSY | IORESOURCE_IO }
};

void __init reserve_standard_io_resources(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(standard_io_resources); i++)
		request_resource(&ioport_resource, &standard_io_resources[i]);

}

#ifdef CONFIG_CRASH_DUMP
 
static int __init setup_elfcorehdr(char *arg)
{
	char *end;
	if (!arg)
		return -EINVAL;
	elfcorehdr_addr = memparse(arg, &end);
	return end > arg ? 0 : -EINVAL;
}
early_param("elfcorehdr", setup_elfcorehdr);
#endif

#ifdef CONFIG_X86_RESERVE_LOW_64K
static int __init dmi_low_memory_corruption(const struct dmi_system_id *d)
{
	printk(KERN_NOTICE
		"%s detected: BIOS may corrupt low RAM, working around it.\n",
		d->ident);

	e820_update_range(0, 0x10000, E820_RAM, E820_RESERVED);
	sanitize_e820_map(e820.map, ARRAY_SIZE(e820.map), &e820.nr_map);

	return 0;
}
#endif

static struct dmi_system_id __initdata bad_bios_dmi_table[] = {
#ifdef CONFIG_X86_RESERVE_LOW_64K
	{
		.callback = dmi_low_memory_corruption,
		.ident = "AMI BIOS",
		.matches = {
			DMI_MATCH(DMI_BIOS_VENDOR, "American Megatrends Inc."),
		},
	},
	{
		.callback = dmi_low_memory_corruption,
		.ident = "Phoenix BIOS",
		.matches = {
			DMI_MATCH(DMI_BIOS_VENDOR, "Phoenix Technologies"),
		},
	},
	{
		.callback = dmi_low_memory_corruption,
		.ident = "Phoenix/MSC BIOS",
		.matches = {
			DMI_MATCH(DMI_BIOS_VENDOR, "Phoenix/MSC"),
		},
	},
	 
	{
		.callback = dmi_low_memory_corruption,
		.ident = "AMI BIOS",
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "DG45ID"),
		},
	},
	{
		.callback = dmi_low_memory_corruption,
		.ident = "AMI BIOS",
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "DG45FC"),
		},
	},
#endif
	{}
};

void __init setup_arch(char **cmdline_p)
{
#ifdef CONFIG_X86_32
	memcpy(&boot_cpu_data, &new_cpu_data, sizeof(new_cpu_data));
	visws_early_detect();
#else
	printk(KERN_INFO "Command line: %s\n", boot_command_line);
#endif

	vmi_init();

	early_cpu_init();
	early_ioremap_init();

	ROOT_DEV = old_decode_dev(boot_params.hdr.root_dev);
	screen_info = boot_params.screen_info;
	edid_info = boot_params.edid_info;
#ifdef CONFIG_X86_32
	apm_info.bios = boot_params.apm_bios_info;
	ist_info = boot_params.ist_info;
	if (boot_params.sys_desc_table.length != 0) {
		set_mca_bus(boot_params.sys_desc_table.table[3] & 0x2);
		machine_id = boot_params.sys_desc_table.table[0];
		machine_submodel_id = boot_params.sys_desc_table.table[1];
		BIOS_revision = boot_params.sys_desc_table.table[2];
	}
#endif
	saved_video_mode = boot_params.hdr.vid_mode;
	bootloader_type = boot_params.hdr.type_of_loader;
	if ((bootloader_type >> 4) == 0xe) {
		bootloader_type &= 0xf;
		bootloader_type |= (boot_params.hdr.ext_loader_type+0x10) << 4;
	}
	bootloader_version  = bootloader_type & 0xf;
	bootloader_version |= boot_params.hdr.ext_loader_ver << 4;

#ifdef CONFIG_BLK_DEV_RAM
	rd_image_start = boot_params.hdr.ram_size & RAMDISK_IMAGE_START_MASK;
	rd_prompt = ((boot_params.hdr.ram_size & RAMDISK_PROMPT_FLAG) != 0);
	rd_doload = ((boot_params.hdr.ram_size & RAMDISK_LOAD_FLAG) != 0);
#endif
#ifdef CONFIG_EFI
	if (!strncmp((char *)&boot_params.efi_info.efi_loader_signature,
#ifdef CONFIG_X86_32
		     "EL32",
#else
		     "EL64",
#endif
	 4)) {
		efi_enabled = 1;
		efi_reserve_early();
	}
#endif

	x86_init.oem.arch_setup();

	setup_memory_map();
	parse_setup_data();
	 
	e820_reserve_setup_data();

	copy_edd();

	if (!boot_params.hdr.root_flags)
		root_mountflags &= ~MS_RDONLY;
	init_mm.start_code = (unsigned long) _text;
	init_mm.end_code = (unsigned long) _etext;
	init_mm.end_data = (unsigned long) _edata;
	init_mm.brk = _brk_end;

	code_resource.start = virt_to_phys(_text);
	code_resource.end = virt_to_phys(_etext)-1;
	data_resource.start = virt_to_phys(_etext);
	data_resource.end = virt_to_phys(_edata)-1;
	bss_resource.start = virt_to_phys(&__bss_start);
	bss_resource.end = virt_to_phys(&__bss_stop)-1;

#ifdef CONFIG_CMDLINE_BOOL
#ifdef CONFIG_CMDLINE_OVERRIDE
	strlcpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
#else
	if (builtin_cmdline[0]) {
		 
		strlcat(builtin_cmdline, " ", COMMAND_LINE_SIZE);
		strlcat(builtin_cmdline, boot_command_line, COMMAND_LINE_SIZE);
		strlcpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
	}
#endif
#endif

	strlcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;

#ifdef CONFIG_X86_64
	 
	check_efer();
#endif

	parse_early_param();

#ifdef CONFIG_X86_64
	check_efer();
#endif

	vmi_activate();

	reserve_early_setup_data();

	if (acpi_mps_check()) {
#ifdef CONFIG_X86_LOCAL_APIC
		disable_apic = 1;
#endif
		setup_clear_cpu_cap(X86_FEATURE_APIC);
	}

#ifdef CONFIG_PCI
	if (pci_early_dump_regs)
		early_dump_pci_devices();
#endif

	finish_e820_parsing();

	if (efi_enabled)
		efi_init();

	dmi_scan_machine();

	dmi_check_system(bad_bios_dmi_table);

	init_hypervisor_platform();

	x86_init.resources.probe_roms();

	insert_resource(&iomem_resource, &code_resource);
	insert_resource(&iomem_resource, &data_resource);
	insert_resource(&iomem_resource, &bss_resource);

#ifdef CONFIG_X86_32
	if (ppro_with_ram_bug()) {
		e820_update_range(0x70000000ULL, 0x40000ULL, E820_RAM,
				  E820_RESERVED);
		sanitize_e820_map(e820.map, ARRAY_SIZE(e820.map), &e820.nr_map);
		printk(KERN_INFO "fixed physical RAM map:\n");
		e820_print_map("bad_ppro");
	}
#else
	early_gart_iommu_check();
#endif

	max_pfn = e820_end_of_ram_pfn();

	early_reserve_e820_mpc_new();
	 
	mtrr_bp_init();
	if (mtrr_trim_uncached_memory(max_pfn))
		max_pfn = e820_end_of_ram_pfn();

#ifdef CONFIG_X86_32
	 
	find_low_pfn_range();
#else
	num_physpages = max_pfn;

	check_x2apic();

	if (max_pfn > (1UL<<(32 - PAGE_SHIFT)))
		max_low_pfn = e820_end_of_low_ram_pfn();
	else
		max_low_pfn = max_pfn;

	high_memory = (void *)__va(max_pfn * PAGE_SIZE - 1) + 1;
	max_pfn_mapped = KERNEL_IMAGE_SIZE >> PAGE_SHIFT;
#endif

#ifdef CONFIG_X86_CHECK_BIOS_CORRUPTION
	setup_bios_corruption_check();
#endif

	printk(KERN_DEBUG "initial memory mapped : 0 - %08lx\n",
			max_pfn_mapped<<PAGE_SHIFT);

	reserve_brk();

	init_gbpages();

	max_low_pfn_mapped = init_memory_mapping(0, max_low_pfn<<PAGE_SHIFT);
	max_pfn_mapped = max_low_pfn_mapped;

#ifdef CONFIG_X86_64
	if (max_pfn > max_low_pfn) {
		max_pfn_mapped = init_memory_mapping(1UL<<32,
						     max_pfn<<PAGE_SHIFT);
		 
		max_low_pfn = max_pfn;
	}
#endif

#ifdef CONFIG_PROVIDE_OHCI1394_DMA_INIT
	if (init_ohci1394_dma_early)
		init_ohci1394_dma_on_all_controllers();
#endif

	reserve_initrd();

	vsmp_init();

	io_delay_init();

	acpi_boot_table_init();

	early_acpi_boot_init();

#ifdef CONFIG_ACPI_NUMA
	 
	acpi_numa_init();
#endif

	initmem_init(0, max_pfn);

#ifdef CONFIG_ACPI_SLEEP
	 
	acpi_reserve_bootmem();
#endif
	 
	find_smp_config();

	reserve_crashkernel();

#ifdef CONFIG_X86_64
	 
	dma32_reserve_bootmem();
#endif

	reserve_ibft_region();

#ifdef CONFIG_KVM_CLOCK
	kvmclock_init();
#endif

	x86_init.paging.pagetable_setup_start(swapper_pg_dir);
	paging_init();
	x86_init.paging.pagetable_setup_done(swapper_pg_dir);

	tboot_probe();

#ifdef CONFIG_X86_64
	map_vsyscall();
#endif

	generic_apic_probe();

	early_quirks();

	acpi_boot_init();

	sfi_init();

	if (smp_found_config)
		get_smp_config();

	prefill_possible_map();

#ifdef CONFIG_X86_64
	init_cpu_to_node();
#endif

	init_apic_mappings();
	ioapic_init_mappings();

	probe_nr_irqs_gsi();

	kvm_guest_init();

	e820_reserve_resources();
	e820_mark_nosave_regions(max_low_pfn);

	x86_init.resources.reserve_resources();

	e820_setup_gap();

#ifdef CONFIG_VT
#if defined(CONFIG_VGA_CONSOLE)
	if (!efi_enabled || (efi_mem_type(0xa0000) != EFI_CONVENTIONAL_MEMORY))
		conswitchp = &vga_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
#endif
	x86_init.oem.banner();

	mcheck_intel_therm_init();
}

#ifdef CONFIG_X86_32

static struct resource video_ram_resource = {
	.name	= "Video RAM area",
	.start	= 0xa0000,
	.end	= 0xbffff,
	.flags	= IORESOURCE_BUSY | IORESOURCE_MEM
};

void __init i386_reserve_resources(void)
{
	request_resource(&iomem_resource, &video_ram_resource);
	reserve_standard_io_resources();
}

#endif  
