#ifndef ___PFE_CTRL_H_
#define ___PFE_CTRL_H_

/** @file
* PFE control code entry points for upper layer linux driver.
*/


/** Control code lower layer command handler.
* Parses and executes commands coming from the upper layers, possibly
* modifying control code internal data structures and well as those used by the PFE engine firmware.
*
*/
void __pfe_ctrl_cmd_handler(u16 fcode, u16 length, u16 *payload, u16 *rlen, u16 *rbuf);


/** Control code lower layer initialization function.
* Initalizes all internal data structures by calling each of the control code modules
* init functions.
*
*/
int __pfe_ctrl_init(void);


/** Control code lower layer cleanup function.
* Frees all the resources allocated by the control code lower layer,
* resets all it's internal data structures and the data structures of the PFE engine firmware
* by calling each of the control code modules exit functions.
*
*/
void __pfe_ctrl_exit(void);

#endif /* ___PFE_CTRL_H_ */
