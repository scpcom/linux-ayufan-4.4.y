#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/types.h>

struct xtm {
	u_int8_t month;    /* (1-12) */
	u_int8_t monthday; /* (1-31) */
	u_int8_t weekday;  /* (1-7) */
	u_int8_t hour;     /* (0-23) */
	u_int8_t minute;   /* (0-59) */
	u_int8_t second;   /* (0-59) */
	unsigned int dse;
};

unsigned int localtime_1(struct xtm *r, time_t time);
void localtime_2(struct xtm *r, time_t time);
void localtime_3(struct xtm *r, time_t time);
