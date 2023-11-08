#ifndef TARGET_CORE_EXTERN_H
#define TARGET_CORE_EXTERN_H
void iblock_update_allocated(struct se_device *dev);
/* 20140422, adamhsu, JS chen, redmine 8042 */
void __update_allocated_attr(struct se_device *dev);

#endif
