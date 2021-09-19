#ifdef CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE
#else
#ifndef _ASM_X86_CMDLINE_H
#define _ASM_X86_CMDLINE_H

int cmdline_find_option_bool(const char *cmdline_ptr, const char *option);
int cmdline_find_option(const char *cmdline_ptr, const char *option,
			char *buffer, int bufsize);

#endif /* _ASM_X86_CMDLINE_H */
#endif	/* CONFIG_SYNO_SKIP_LK3_10_KPTI_RETPOLINE */
