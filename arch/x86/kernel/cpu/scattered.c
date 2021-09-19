/*
 *	Routines to indentify additional cpu features that are scattered in
 *	cpuid space.
 */
#include <linux/cpu.h>

#include <asm/pat.h>
#include <asm/processor.h>

#include <asm/apic.h>

struct cpuid_bit {
	u16 feature;
	u8 reg;
	u8 bit;
	u32 level;
	u32 sub_leaf;
};

#ifdef CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE
enum cpuid_regs {
	CR_EAX = 0,
	CR_ECX,
	CR_EDX,
	CR_EBX
};

void __cpuinit init_scattered_cpuid_features(struct cpuinfo_x86 *c)
{
	u32 max_level;
	u32 regs[4];
	const struct cpuid_bit *cb;

	static const struct cpuid_bit __cpuinitconst cpuid_bits[] = {
		{ X86_FEATURE_DTHERM,		CR_EAX, 0, 0x00000006, 0 },
		{ X86_FEATURE_IDA,		CR_EAX, 1, 0x00000006, 0 },
		{ X86_FEATURE_ARAT,		CR_EAX, 2, 0x00000006, 0 },
		{ X86_FEATURE_PLN,		CR_EAX, 4, 0x00000006, 0 },
		{ X86_FEATURE_PTS,		CR_EAX, 6, 0x00000006, 0 },
		{ X86_FEATURE_APERFMPERF,	CR_ECX, 0, 0x00000006, 0 },
		{ X86_FEATURE_EPB,		CR_ECX, 3, 0x00000006, 0 },
		{ X86_FEATURE_XSAVEOPT,		CR_EAX,	0, 0x0000000d, 1 },
		{ X86_FEATURE_HW_PSTATE,	CR_EDX, 7, 0x80000007, 0 },
		{ X86_FEATURE_CPB,		CR_EDX, 9, 0x80000007, 0 },
		{ X86_FEATURE_PROC_FEEDBACK,	CR_EDX,11, 0x80000007, 0 },
		{ X86_FEATURE_NPT,		CR_EDX, 0, 0x8000000a, 0 },
		{ X86_FEATURE_LBRV,		CR_EDX, 1, 0x8000000a, 0 },
		{ X86_FEATURE_SVML,		CR_EDX, 2, 0x8000000a, 0 },
		{ X86_FEATURE_NRIPS,		CR_EDX, 3, 0x8000000a, 0 },
		{ X86_FEATURE_TSCRATEMSR,	CR_EDX, 4, 0x8000000a, 0 },
		{ X86_FEATURE_VMCBCLEAN,	CR_EDX, 5, 0x8000000a, 0 },
		{ X86_FEATURE_FLUSHBYASID,	CR_EDX, 6, 0x8000000a, 0 },
		{ X86_FEATURE_DECODEASSISTS,	CR_EDX, 7, 0x8000000a, 0 },
		{ X86_FEATURE_PAUSEFILTER,	CR_EDX,10, 0x8000000a, 0 },
		{ X86_FEATURE_PFTHRESHOLD,	CR_EDX,12, 0x8000000a, 0 },
		{ 0, 0, 0, 0, 0 }
	};

	for (cb = cpuid_bits; cb->feature; cb++) {

		/* Verify that the level is valid */
		max_level = cpuid_eax(cb->level & 0xffff0000);
		if (max_level < cb->level ||
		    max_level > (cb->level | 0xffff))
			continue;

		cpuid_count(cb->level, cb->sub_leaf, &regs[CR_EAX],
			    &regs[CR_EBX], &regs[CR_ECX], &regs[CR_EDX]);

		if (regs[cb->reg] & (1 << cb->bit))
			set_cpu_cap(c, cb->feature);
	}
}
#else
static const struct cpuid_bit __cpuinitconst cpuid_bits[] = {
	{ X86_FEATURE_DTHERM,		CPUID_EAX, 0, 0x00000006, 0 },
	{ X86_FEATURE_IDA,		CPUID_EAX, 1, 0x00000006, 0 },
	{ X86_FEATURE_ARAT,		CPUID_EAX, 2, 0x00000006, 0 },
	{ X86_FEATURE_PLN,		CPUID_EAX, 4, 0x00000006, 0 },
	{ X86_FEATURE_PTS,		CPUID_EAX, 6, 0x00000006, 0 },
	{ X86_FEATURE_SPEC_CTRL,        CPUID_EDX, 26, 0x00000007, 0 },
	{ X86_FEATURE_APERFMPERF,	CPUID_ECX, 0, 0x00000006, 0 },
	{ X86_FEATURE_EPB,		CPUID_ECX, 3, 0x00000006, 0 },
	{ X86_FEATURE_XSAVEOPT,		CPUID_EAX,	0, 0x0000000d, 1 },
	{ X86_FEATURE_HW_PSTATE,	CPUID_EDX, 7, 0x80000007, 0 },
	{ X86_FEATURE_CPB,		CPUID_EDX, 9, 0x80000007, 0 },
	{ X86_FEATURE_PROC_FEEDBACK,	CPUID_EDX,11, 0x80000007, 0 },
	{ X86_FEATURE_NPT,		CPUID_EDX, 0, 0x8000000a, 0 },
	{ X86_FEATURE_LBRV,		CPUID_EDX, 1, 0x8000000a, 0 },
	{ X86_FEATURE_SVML,		CPUID_EDX, 2, 0x8000000a, 0 },
	{ X86_FEATURE_NRIPS,		CPUID_EDX, 3, 0x8000000a, 0 },
	{ X86_FEATURE_TSCRATEMSR,	CPUID_EDX, 4, 0x8000000a, 0 },
	{ X86_FEATURE_VMCBCLEAN,	CPUID_EDX, 5, 0x8000000a, 0 },
	{ X86_FEATURE_FLUSHBYASID,	CPUID_EDX, 6, 0x8000000a, 0 },
	{ X86_FEATURE_DECODEASSISTS,	CPUID_EDX, 7, 0x8000000a, 0 },
	{ X86_FEATURE_PAUSEFILTER,	CPUID_EDX,10, 0x8000000a, 0 },
	{ X86_FEATURE_PFTHRESHOLD,	CPUID_EDX,12, 0x8000000a, 0 },
	{ X86_FEATURE_IBPB_SUPPORT,     CPUID_EBX,12, 0x80000008, 0 },
	{ 0, 0, 0, 0, 0 }
};

void __cpuinit init_scattered_cpuid_features(struct cpuinfo_x86 *c)
{
	u32 max_level;
	u32 regs[4];
	const struct cpuid_bit *cb;

	for (cb = cpuid_bits; cb->feature; cb++) {

		/* Verify that the level is valid */
		max_level = cpuid_eax(cb->level & 0xffff0000);
		if (max_level < cb->level ||
		    max_level > (cb->level | 0xffff))
			continue;

		cpuid_count(cb->level, cb->sub_leaf, &regs[CPUID_EAX],
			    &regs[CPUID_EBX], &regs[CPUID_ECX],
				&regs[CPUID_EDX]);

		if (regs[cb->reg] & (1 << cb->bit))
			set_cpu_cap(c, cb->feature);
	}

	if (cpu_has(c, X86_FEATURE_SPEC_CTRL))
		set_cpu_cap(c, X86_FEATURE_IBPB_SUPPORT);
}

u32 get_scattered_cpuid_leaf(unsigned int level, unsigned int sub_leaf,
			     enum cpuid_regs_idx reg)
{
	const struct cpuid_bit *cb;
	u32 cpuid_val = 0;

	for (cb = cpuid_bits; cb->feature; cb++) {

		if (level > cb->level)
			continue;

		if (level < cb->level)
			break;

		if (reg == cb->reg && sub_leaf == cb->sub_leaf) {
			if (cpu_has(&boot_cpu_data, cb->feature))
				cpuid_val |= BIT(cb->bit);
		}
	}

	return cpuid_val;
}
EXPORT_SYMBOL_GPL(get_scattered_cpuid_leaf);
#endif	/* CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE */
