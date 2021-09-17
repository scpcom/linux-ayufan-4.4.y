#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef __OF_IRQ_H
#define __OF_IRQ_H

#if defined(CONFIG_OF) || defined(MY_ABC_HERE)
#if defined(MY_ABC_HERE)
 
#else  
struct of_irq;
#endif  
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/ioport.h>
#include <linux/of.h>

#if defined(MY_ABC_HERE)
 
#else  
 
extern unsigned int irq_of_parse_and_map(struct device_node *node, int index);

#if defined(CONFIG_OF_IRQ)
 
#define OF_MAX_IRQ_SPEC		4  
struct of_irq {
	struct device_node *controller;  
	u32 size;  
	u32 specifier[OF_MAX_IRQ_SPEC];  
};
#endif  
#endif  

#if defined(CONFIG_OF_IRQ) || defined(MY_ABC_HERE)
typedef int (*of_irq_init_cb_t)(struct device_node *, struct device_node *);

#define OF_IMAP_OLDWORLD_MAC	0x00000001
#define OF_IMAP_NO_PHANDLE	0x00000002

#if defined(CONFIG_PPC32) && defined(CONFIG_PPC_PMAC)
extern unsigned int of_irq_workarounds;
extern struct device_node *of_irq_dflt_pic;
#if defined(MY_ABC_HERE)
extern int of_irq_parse_oldworld(struct device_node *device, int index,
			       struct of_phandle_args *out_irq);
#else  
extern int of_irq_map_oldworld(struct device_node *device, int index,
			       struct of_irq *out_irq);
#endif  
#else  
#define of_irq_workarounds (0)
#define of_irq_dflt_pic (NULL)
#if defined(MY_ABC_HERE)
static inline int of_irq_parse_oldworld(struct device_node *device, int index,
				      struct of_phandle_args *out_irq)
#else  
static inline int of_irq_map_oldworld(struct device_node *device, int index,
				      struct of_irq *out_irq)
#endif  
{
	return -EINVAL;
}
#endif  

#if defined(MY_ABC_HERE)
extern int of_irq_parse_raw(const __be32 *addr, struct of_phandle_args *out_irq);
extern int of_irq_parse_one(struct device_node *device, int index,
			  struct of_phandle_args *out_irq);
extern unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data);
#else  
extern int of_irq_map_raw(struct device_node *parent, const __be32 *intspec,
			  u32 ointsize, const __be32 *addr,
			  struct of_irq *out_irq);
extern int of_irq_map_one(struct device_node *device, int index,
			  struct of_irq *out_irq);
extern unsigned int irq_create_of_mapping(struct device_node *controller,
					  const u32 *intspec,
					  unsigned int intsize);
#endif  
extern int of_irq_to_resource(struct device_node *dev, int index,
			      struct resource *r);
extern int of_irq_count(struct device_node *dev);
extern int of_irq_to_resource_table(struct device_node *dev,
		struct resource *res, int nr_irqs);
#if defined(MY_ABC_HERE)
 
#else  
extern struct device_node *of_irq_find_parent(struct device_node *child);
#endif  

extern void of_irq_init(const struct of_device_id *matches);

#endif  

#if defined(MY_ABC_HERE) && defined(CONFIG_OF)
 
extern unsigned int irq_of_parse_and_map(struct device_node *node, int index);
extern struct device_node *of_irq_find_parent(struct device_node *child);
#endif  

#else  
static inline unsigned int irq_of_parse_and_map(struct device_node *dev,
						int index)
{
	return 0;
}

static inline void *of_irq_find_parent(struct device_node *child)
{
	return NULL;
}
#endif  

#endif  
