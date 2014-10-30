#include <linux/reboot.h>

extern void ls1024a_secondary_startup(void);
extern void c2k_local_timer_init(void);
void c2k_init_early(void);
void c2k_init_late(void);
void c2k_restart(enum reboot_mode mode, const char *cmd);
void __init c2k_map_io(void);
void c2k_gic_of_init(void);
extern void c2k_reserve(void);
