#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _ASM_X86_MCE_H
#define _ASM_X86_MCE_H

#include <uapi/asm/mce.h>

#define MCG_BANKCNT_MASK	0xff          
#define MCG_CTL_P		(1ULL<<8)     
#define MCG_EXT_P		(1ULL<<9)     
#define MCG_CMCI_P		(1ULL<<10)    
#define MCG_EXT_CNT_MASK	0xff0000      
#define MCG_EXT_CNT_SHIFT	16
#define MCG_EXT_CNT(c)		(((c) & MCG_EXT_CNT_MASK) >> MCG_EXT_CNT_SHIFT)
#define MCG_SER_P		(1ULL<<24)    

#define MCG_STATUS_RIPV  (1ULL<<0)    
#define MCG_STATUS_EIPV  (1ULL<<1)    
#define MCG_STATUS_MCIP  (1ULL<<2)    

#define MCI_STATUS_VAL   (1ULL<<63)   
#define MCI_STATUS_OVER  (1ULL<<62)   
#define MCI_STATUS_UC    (1ULL<<61)   
#define MCI_STATUS_EN    (1ULL<<60)   
#define MCI_STATUS_MISCV (1ULL<<59)   
#define MCI_STATUS_ADDRV (1ULL<<58)   
#define MCI_STATUS_PCC   (1ULL<<57)   
#define MCI_STATUS_S	 (1ULL<<56)   
#define MCI_STATUS_AR	 (1ULL<<55)   
#ifdef MY_DEF_HERE  
#define SYNO_MCI_STATUS_UECC_SHIFT 45
#define SYNO_MCI_STATUS_CECC_SHIFT 46
#define SYNO_MCI_STATUS_ECC_SYNDROME_SHIFT 47
#define SYNO_MCI_STATUS_UECC (1ULL << SYNO_MCI_STATUS_UECC_SHIFT)        
#define SYNO_MCI_STATUS_CECC (1ULL << SYNO_MCI_STATUS_CECC_SHIFT)        
#define SYNO_MCI_STATUS_ECC (SYNO_MCI_STATUS_UECC | SYNO_MCI_STATUS_CECC)
#define SYNO_MCI_STATUS_ECC_SYNDROME (0xffULL << SYNO_MCI_STATUS_ECC_SYNDROME_SHIFT)  
#endif  

#define MCACOD		  0xefff      

#define MCACOD_SCRUB	0x00C0	 
#define MCACOD_SCRUBMSK	0xeff0	 
#define MCACOD_L3WB	0x017A	 
#define MCACOD_DATA	0x0134	 
#define MCACOD_INSTR	0x0150	 

#define MCI_MISC_ADDR_LSB(m)	((m) & 0x3f)
#define MCI_MISC_ADDR_MODE(m)	(((m) >> 6) & 7)
#define  MCI_MISC_ADDR_SEGOFF	0	 
#define  MCI_MISC_ADDR_LINEAR	1	 
#define  MCI_MISC_ADDR_PHYS	2	 
#define  MCI_MISC_ADDR_MEM	3	 
#define  MCI_MISC_ADDR_GENERIC	7	 

#define MCI_CTL2_CMCI_EN		(1ULL << 30)
#define MCI_CTL2_CMCI_THRESHOLD_MASK	0x7fffULL

#define MCJ_CTX_MASK		3
#define MCJ_CTX(flags)		((flags) & MCJ_CTX_MASK)
#define MCJ_CTX_RANDOM		0     
#define MCJ_CTX_PROCESS		0x1   
#define MCJ_CTX_IRQ		0x2   
#define MCJ_NMI_BROADCAST	0x4   
#define MCJ_EXCEPTION		0x8   
#define MCJ_IRQ_BRAODCAST	0x10  

#define MCE_OVERFLOW 0		 

#define MCE_EXTENDED_BANK	128
#define MCE_THERMAL_BANK	(MCE_EXTENDED_BANK + 0)
#define K8_MCE_THRESHOLD_BASE   (MCE_EXTENDED_BANK + 1)

#define MCE_LOG_LEN 32
#define MCE_LOG_SIGNATURE	"MACHINECHECK"

struct mce_log {
	char signature[12];  
	unsigned len;	     
	unsigned next;
	unsigned flags;
	unsigned recordlen;	 
	struct mce entry[MCE_LOG_LEN];
};

struct mca_config {
	bool dont_log_ce;
	bool cmci_disabled;
	bool ignore_ce;
	bool disabled;
	bool ser;
	bool bios_cmci_threshold;
	u8 banks;
	s8 bootlog;
	int tolerant;
	int monarch_timeout;
	int panic_timeout;
	u32 rip_msr;
};

extern struct mca_config mca_cfg;
extern void mce_register_decode_chain(struct notifier_block *nb);
extern void mce_unregister_decode_chain(struct notifier_block *nb);

#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/atomic.h>

extern int mce_p5_enabled;

#ifdef CONFIG_X86_MCE
int mcheck_init(void);
void mcheck_cpu_init(struct cpuinfo_x86 *c);
#else
static inline int mcheck_init(void) { return 0; }
static inline void mcheck_cpu_init(struct cpuinfo_x86 *c) {}
#endif

#ifdef CONFIG_X86_ANCIENT_MCE
void intel_p5_mcheck_init(struct cpuinfo_x86 *c);
void winchip_mcheck_init(struct cpuinfo_x86 *c);
static inline void enable_p5_mce(void) { mce_p5_enabled = 1; }
#else
static inline void intel_p5_mcheck_init(struct cpuinfo_x86 *c) {}
static inline void winchip_mcheck_init(struct cpuinfo_x86 *c) {}
static inline void enable_p5_mce(void) {}
#endif

void mce_setup(struct mce *m);
void mce_log(struct mce *m);
DECLARE_PER_CPU(struct device *, mce_device);

#define MAX_NR_BANKS 32

#ifdef CONFIG_X86_MCE_INTEL
void mce_intel_feature_init(struct cpuinfo_x86 *c);
void cmci_clear(void);
void cmci_reenable(void);
void cmci_rediscover(void);
void cmci_recheck(void);
#else
static inline void mce_intel_feature_init(struct cpuinfo_x86 *c) { }
static inline void cmci_clear(void) {}
static inline void cmci_reenable(void) {}
static inline void cmci_rediscover(void) {}
static inline void cmci_recheck(void) {}
#endif

#ifdef CONFIG_X86_MCE_AMD
void mce_amd_feature_init(struct cpuinfo_x86 *c);
#else
static inline void mce_amd_feature_init(struct cpuinfo_x86 *c) { }
#endif

int mce_available(struct cpuinfo_x86 *c);

DECLARE_PER_CPU(unsigned, mce_exception_count);
DECLARE_PER_CPU(unsigned, mce_poll_count);

extern atomic_t mce_entry;

typedef DECLARE_BITMAP(mce_banks_t, MAX_NR_BANKS);
DECLARE_PER_CPU(mce_banks_t, mce_poll_banks);

enum mcp_flags {
	MCP_TIMESTAMP = (1 << 0),	 
	MCP_UC = (1 << 1),		 
	MCP_DONTLOG = (1 << 2),		 
};
void machine_check_poll(enum mcp_flags flags, mce_banks_t *b);

int mce_notify_irq(void);
void mce_notify_process(void);

DECLARE_PER_CPU(struct mce, injectm);

extern void register_mce_write_callback(ssize_t (*)(struct file *filp,
				    const char __user *ubuf,
				    size_t usize, loff_t *off));

extern void (*machine_check_vector)(struct pt_regs *, long error_code);
void do_machine_check(struct pt_regs *, long);

extern void (*mce_threshold_vector)(void);
extern void (*threshold_cpu_callback)(unsigned long action, unsigned int cpu);

void intel_init_thermal(struct cpuinfo_x86 *c);

void mce_log_therm_throt_event(__u64 status);

extern int (*platform_thermal_notify)(__u64 msr_val);

#ifdef CONFIG_X86_THERMAL_VECTOR
extern void mcheck_intel_therm_init(void);
#else
static inline void mcheck_intel_therm_init(void) { }
#endif

struct cper_sec_mem_err;
extern void apei_mce_report_mem_error(int corrected,
				      struct cper_sec_mem_err *mem_err);

#endif  
