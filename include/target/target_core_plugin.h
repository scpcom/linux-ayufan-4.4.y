#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef TARGET_CORE_PLUGIN_H
#define TARGET_CORE_PLUGIN_H

#define PLUGIN_TYPE_TRANSPORT	0
#ifndef MY_ABC_HERE
#define PLUGIN_TYPE_OBJ		1
#endif

#define MAX_PLUGINS		32
#define MAX_PLUGIN_CLASSES	16
#define MAX_PLUGIN_CLASS_NAME	16
#define MAX_PLUGIN_NAME		16

#define PLUGIN_FREE		0
#define PLUGIN_REGISTERED	1

extern se_global_t *se_global;

extern void plugin_load_all_classes(void);
extern struct se_plugin_class_s *plugin_get_class(u32);
extern int plugin_register_class(u32, unsigned char *, int);
extern int plugin_deregister_class(u32);
extern void plugin_unload_all_classes(void);
extern void *plugin_get_obj(u32, u32, int *);
extern struct se_plugin_s *plugin_register(void *, u32, unsigned char *, u32,
				void (*get_plugin_info)(void *, char *, int *),
				int (*plugin_init)(void),
				void (*plugin_free)(void), int *);
extern int plugin_deregister(u32, u32);

typedef struct se_plugin_class_s {
	unsigned char		plugin_class_name[MAX_PLUGIN_CLASS_NAME];
	u32			plugin_class;
	u32			max_plugins;
	struct se_plugin_s	*plugin_array;
	spinlock_t		plugin_lock;
} se_plugin_class_t;

typedef struct se_plugin_s	{
	unsigned char		plugin_name[MAX_PLUGIN_NAME];
	int			plugin_state;
	u32			plugin_type;
	se_plugin_class_t	*plugin_class;
	void			*plugin_obj;
	void (*get_plugin_info)(void *, char *, int *);
	void (*plugin_free)(void);
} se_plugin_t;

#endif  
