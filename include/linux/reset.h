#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_RESET_H_
#define _LINUX_RESET_H_

struct device;
struct reset_control;
#if defined(MY_ABC_HERE)
struct device_node;
#endif  

int reset_control_reset(struct reset_control *rstc);
int reset_control_assert(struct reset_control *rstc);
int reset_control_deassert(struct reset_control *rstc);
#if defined (MY_DEF_HERE)
int reset_control_is_asserted(struct reset_control *rstc);
#endif  

#if defined(MY_ABC_HERE)
struct reset_control *of_reset_control_get(struct device_node *np, const char *id);
#endif  
struct reset_control *reset_control_get(struct device *dev, const char *id);
void reset_control_put(struct reset_control *rstc);
struct reset_control *devm_reset_control_get(struct device *dev, const char *id);

int device_reset(struct device *dev);

#endif
