#ifndef __xorh
#define __xorh

#ifdef __cplusplus
extern "C" {
#endif

/* typedefs */

/* This enumerator describes the set of commands that can be applied on
 an engine (e.g. IDMA, XOR). Appling a comman depends on the current
 status (see XOR_STATE enumerator)
 Start can be applied only when status is IDLE
 Stop can be applied only when status is IDLE, ACTIVE or PAUSED
 Pause can be applied only when status is ACTIVE
 Restart can be applied only when status is PAUSED		*/
typedef enum _xor_command {
	XOR_START,              /* Start	*/
	XOR_STOP,               /* Stop		*/
	XOR_PAUSE,              /* Pause	*/
	XOR_RESTART             /* Restart	*/
} XOR_COMMAND;

/* This enumerator describes the set of state conditions.
   Moving from one state to other is stricted. */
typedef enum _xor_state {
	XOR_IDLE,
	XOR_ACTIVE,
	XOR_PAUSED,
	XOR_UNDEFINED_STATE
} XOR_STATE;

/* This enumerator describes the type of functionality the XOR channel
   can have while using the same data structures.*/
typedef enum _xortype {
	XOR_XOR,     /* XOR channel functions as XOR accelerator */
	XOR_DMA,     /* XOR channel functions as IDMA channel */
} XOR_TYPE;

/* This structure describes XOR descriptor size 64bytes */
typedef struct _xordesc {
	/* Successful descriptor execution indication */
	unsigned int status;
	/* Result of CRC-32 calculation */
	unsigned int crc32result;
	/* type of operation to be carried out on the data */
	unsigned int desccommand;
	/* Next descriptor address pointer */
	unsigned int phynextdescptr;
	/* Size of source and destination blocks in bytes */
	unsigned int bytecnt;
	/* Destination Block address pointer */
	unsigned int phydestadd;
	/* source block #0 address pointer */
	unsigned int srcadd0;
	/* source block #1 address pointer */
	unsigned int srcadd1;
	/* source block #2 address pointer */
	unsigned int srcadd2;
	/* source block #3 address pointer */
	unsigned int srcadd3;
	/* source block #4 address pointer */
	unsigned int srcadd4;
	/* source block #5 address pointer */
	unsigned int srcadd5;
	/* source block #6 address pointer */
	unsigned int srcadd6;
	/* source block #7 address pointer */
	unsigned int srcadd7;
	/* source block #8 address pointer */
	unsigned int srcadd8;
	unsigned int reserved0;
} XOR_DESC;
#endif
