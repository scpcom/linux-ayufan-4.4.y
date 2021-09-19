#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  Copyright (C) 1994  Linus Torvalds
 *
 *  Cyrix stuff, June 1998 by:
 *	- Rafael R. Reilova (moved everything from head.S),
 *        <rreilova@ececs.uc.edu>
 *	- Channing Corn (tests & fixes),
 *	- Andrew D. Balsa (code cleanup).
 */
#include <linux/init.h>
#include <linux/utsname.h>
#include <asm/bugs.h>
#include <asm/processor.h>
#include <asm/processor-flags.h>
#include <asm/i387.h>
#include <asm/msr.h>
#include <asm/paravirt.h>
#include <asm/alternative.h>
#ifdef CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE
#else
#include <linux/cpu.h>

#include <asm/cmdline.h>
#include <asm/nospec-branch.h>
#include <asm/pgtable.h>
#include <asm/cacheflush.h>
#include <asm/spec_ctrl.h>
#endif	/* CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE */

#ifdef CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE
static int __init no_387(char *s)
{
	boot_cpu_data.hard_math = 0;
	write_cr0(X86_CR0_TS | X86_CR0_EM | X86_CR0_MP | read_cr0());
	return 1;
}

__setup("no387", no_387);

static double __initdata x = 4195835.0;
static double __initdata y = 3145727.0;

/*
 * This used to check for exceptions..
 * However, it turns out that to support that,
 * the XMM trap handlers basically had to
 * be buggy. So let's have a correct XMM trap
 * handler, and forget about printing out
 * some status at boot.
 *
 * We should really only care about bugs here
 * anyway. Not features.
 */
static void __init check_fpu(void)
{
	s32 fdiv_bug;

	if (!boot_cpu_data.hard_math) {
#ifndef CONFIG_MATH_EMULATION
		pr_emerg("No coprocessor found and no math emulation present\n");
		pr_emerg("Giving up\n");
		for (;;) ;
#endif
		return;
	}

	kernel_fpu_begin();

	/*
	 * trap_init() enabled FXSR and company _before_ testing for FP
	 * problems here.
	 *
	 * Test for the divl bug: http://en.wikipedia.org/wiki/Fdiv_bug
	 */
	__asm__("fninit\n\t"
		"fldl %1\n\t"
		"fdivl %2\n\t"
		"fmull %2\n\t"
		"fldl %1\n\t"
		"fsubp %%st,%%st(1)\n\t"
		"fistpl %0\n\t"
		"fwait\n\t"
		"fninit"
		: "=m" (*&fdiv_bug)
		: "m" (*&x), "m" (*&y));

	kernel_fpu_end();

	if (fdiv_bug) {
		set_cpu_bug(&boot_cpu_data, X86_BUG_FDIV);
		pr_warn("Hmm, FPU with FDIV bug\n");
	}
}

void __init check_bugs(void)
{
	identify_boot_cpu();
#ifndef CONFIG_SMP
	pr_info("CPU: ");
	print_cpu_info(&boot_cpu_data);
#endif

	/*
	 * Check whether we are able to run this kernel safely on SMP.
	 *
	 * - i386 is no longer supported.
	 * - In order to run on anything without a TSC, we need to be
	 *   compiled for a i486.
	 */
	if (boot_cpu_data.x86 < 4)
		panic("Kernel requires i486+ for 'invlpg' and other features");

	init_utsname()->machine[1] =
		'0' + (boot_cpu_data.x86 > 6 ? 6 : boot_cpu_data.x86);
	alternative_instructions();

	/*
	 * kernel_fpu_begin/end() in check_fpu() relies on the patched
	 * alternative instructions.
	 */
	check_fpu();
}
#else
static void __init spectre_v2_select_mitigation(void);

void __init check_bugs(void)
{
	identify_boot_cpu();

	if (!IS_ENABLED(CONFIG_SMP)) {
		pr_info("CPU: ");
		print_cpu_info(&boot_cpu_data);
	}

	spec_ctrl_init();
	spectre_v2_select_mitigation();
	spec_ctrl_cpu_init();

#ifdef CONFIG_X86_32
	/*
	 * Check whether we are able to run this kernel safely on SMP.
	 *
	 * - i386 is no longer supported.
	 * - In order to run on anything without a TSC, we need to be
	 *   compiled for a i486.
	 */
	if (boot_cpu_data.x86 < 4)
		panic("Kernel requires i486+ for 'invlpg' and other features");

	init_utsname()->machine[1] =
		'0' + (boot_cpu_data.x86 > 6 ? 6 : boot_cpu_data.x86);
	alternative_instructions();

	/*
	 * kernel_fpu_begin/end() in check_fpu() relies on the patched
	 * alternative instructions.
	 */
	check_fpu();
	#else /* CONFIG_X86_64 */
	alternative_instructions();

	/*
	 * Make sure the first 2MB area is not mapped by huge pages
	 * There are typically fixed size MTRRs in there and overlapping
	 * MTRRs into large pages causes slow downs.
	 *
	 * Right now we don't do that with gbpages because there seems
	 * very little benefit for that case.
	 */
	if (!direct_gbpages)
		set_memory_4k((unsigned long)__va(0), 1);
#endif
}

/* The kernel command line selection */
enum spectre_v2_mitigation_cmd {
	SPECTRE_V2_CMD_NONE,
	SPECTRE_V2_CMD_FORCE,
	SPECTRE_V2_CMD_AUTO,
	SPECTRE_V2_CMD_RETPOLINE,
	SPECTRE_V2_CMD_RETPOLINE_IBRS_USER,
	SPECTRE_V2_CMD_IBRS,
	SPECTRE_V2_CMD_IBRS_ALWAYS,
};

static const char *spectre_v2_strings[] = {
	[SPECTRE_V2_NONE]			= "Vulnerable",
	[SPECTRE_V2_RETPOLINE_MINIMAL]		= "Vulnerable: Minimal ASM retpoline",
	[SPECTRE_V2_RETPOLINE_NO_IBPB]		= "Vulnerable: Retpoline without IBPB",
	[SPECTRE_V2_RETPOLINE_SKYLAKE]		= "Vulnerable: Retpoline on Skylake+",
	[SPECTRE_V2_RETPOLINE_UNSAFE_MODULE]	= "Vulnerable: Retpoline with unsafe module(s)",
	[SPECTRE_V2_RETPOLINE]			= "Mitigation: Full retpoline",
	[SPECTRE_V2_RETPOLINE_IBRS_USER]	= "Mitigation: Full retpoline and IBRS (user space)",
	[SPECTRE_V2_IBRS]			= "Mitigation: IBRS (kernel)",
	[SPECTRE_V2_IBRS_ALWAYS]		= "Mitigation: IBRS (kernel and user space)",
	[SPECTRE_V2_IBP_DISABLED]		= "Mitigation: IBP disabled",
};

enum spectre_v2_mitigation_cmd spectre_v2_cmd = SPECTRE_V2_CMD_AUTO;

#undef pr_fmt
#define pr_fmt(fmt)     "Spectre V2 : " fmt

static inline bool match_option(const char *arg, int arglen, const char *opt)
{
	int len = strlen(opt);

	return len == arglen && !strncmp(arg, opt, len);
}

static enum spectre_v2_mitigation_cmd spectre_v2_parse_cmdline(void)
{
	char arg[20];
	int ret;

#ifdef MY_DEF_HERE
	if (!cmdline_find_option_bool(boot_command_line, "KPTI_on"))
		goto disable;
#endif /*MY_DEF_HERE*/

	ret = cmdline_find_option(boot_command_line, "spectre_v2", arg,
				  sizeof(arg));
	if (ret > 0)  {
		if (match_option(arg, ret, "off")) {
			goto disable;
		} else if (match_option(arg, ret, "on")) {
			return SPECTRE_V2_CMD_FORCE;
		} else if (match_option(arg, ret, "retpoline")) {
			return SPECTRE_V2_CMD_RETPOLINE;
		} else if (match_option(arg, ret, "retpoline,ibrs_user")) {
			return SPECTRE_V2_CMD_RETPOLINE_IBRS_USER;
		} else if (match_option(arg, ret, "ibrs")) {
			return SPECTRE_V2_CMD_IBRS;
		} else if (match_option(arg, ret, "ibrs_always")) {
			return SPECTRE_V2_CMD_IBRS_ALWAYS;
		} else if (match_option(arg, ret, "auto")) {
			return SPECTRE_V2_CMD_AUTO;
		}
	}

	if (!cmdline_find_option_bool(boot_command_line, "nospectre_v2"))
		return SPECTRE_V2_CMD_AUTO;
disable:
	return SPECTRE_V2_CMD_NONE;
}

void __spectre_v2_select_mitigation(void)
{
	const bool full_retpoline = IS_ENABLED(CONFIG_RETPOLINE) && retp_compiler();
	enum spectre_v2_mitigation_cmd cmd = spectre_v2_cmd;

	switch (cmd) {
	case SPECTRE_V2_CMD_NONE:
		return;

	case SPECTRE_V2_CMD_FORCE:
	case SPECTRE_V2_CMD_AUTO:
		break;

	case SPECTRE_V2_CMD_RETPOLINE:
		spec_ctrl_enable_retpoline();
		return;

	case SPECTRE_V2_CMD_IBRS:
		if (spec_ctrl_force_enable_ibrs())
			return;
		break;

	case SPECTRE_V2_CMD_IBRS_ALWAYS:
		if (spec_ctrl_enable_ibrs_always() ||
		    spec_ctrl_force_enable_ibp_disabled())
			return;
		break;

	case SPECTRE_V2_CMD_RETPOLINE_IBRS_USER:
		if (spec_ctrl_enable_retpoline_ibrs_user())
			return;
		break;
	}

	if (spec_ctrl_cond_enable_ibrs(full_retpoline))
		return;

	if (spec_ctrl_cond_enable_ibp_disabled())
		return;

	spec_ctrl_enable_retpoline();
}

void spectre_v2_print_mitigation(void)
{

	pr_info("%s\n", spectre_v2_strings[spec_ctrl_get_mitigation()]);
}

static void __init spectre_v2_select_mitigation(void)
{
	spectre_v2_cmd = spectre_v2_parse_cmdline();
	__spectre_v2_select_mitigation();
	spectre_v2_print_mitigation();
}

#undef pr_fmt

#ifdef CONFIG_SYSFS
ssize_t cpu_show_meltdown(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	if (boot_cpu_data.x86_vendor == X86_VENDOR_AMD)
		return sprintf(buf, "Not affected\n");
#ifdef CONFIG_KAISER
	if (kaiser_enabled)
		return sprintf(buf, "Mitigation: PTI\n");
#endif /* CONFIG_KAISER */
	return sprintf(buf, "Vulnerable\n");
}

ssize_t cpu_show_spectre_v1(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Mitigation: Load fences\n");
}

ssize_t cpu_show_spectre_v2(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", spectre_v2_strings[spec_ctrl_get_mitigation()]);
}
#endif
#endif	/* CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE */
