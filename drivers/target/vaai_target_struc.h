/**
 * @file 	vaai_target_struc.h
 * @brief	Basic strcuture declaration header file
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */



#ifndef _TARGET_STRUCT_H_
#define _TARGET_STRUCT_H_

#include "vaai_comp_opt.h"

/**/
typedef struct se_subsystem_dev     LIO_SE_SUBSYSTEM_DEV;
typedef struct se_task              LIO_SE_TASK;
typedef struct se_cmd               LIO_SE_CMD;
typedef struct se_device            LIO_SE_DEVICE;
typedef struct se_subsystem_api     LIO_SE_SUBSYSTEM_API;
typedef struct se_hba               LIO_SE_HBA;
typedef struct iblock_dev           LIO_IBLOCK_DEV;
typedef struct fd_dev               LIO_FD_DEV;
typedef struct se_node_acl          LIO_SE_NODE_ACL;
typedef struct se_dev_entry         LIO_SE_DEVENTRY;
typedef struct se_session           LIO_SE_SESSION;
typedef struct se_lun               LIO_SE_LUN;
typedef struct se_port              LIO_SE_PORT;
typedef struct se_portal_group      LIO_SE_PORTAL_GROUP;


#ifndef T_TASK
#define T_TASK(Cmd)        ((LIO_SE_CMD *)Cmd)
#endif

#define CMD_TO_TASK_CDB(Cmd)    T_TASK(Cmd)


#endif /* _TARGET_STRUCT_H_ */

