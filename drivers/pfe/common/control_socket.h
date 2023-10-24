/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#include "module_socket.h"


PSockEntry SOCKET_bind(U16 socketID, PVOID owner, U16 owner_type)__attribute__ ((noinline));
PSockEntry SOCKET_unbind(U16 socketID) __attribute__ ((noinline));

void SOCKET4_free_entries(void);
void SOCKET4_delete_route(PSockEntry pSocket);

int SOCKET4_HandleIP_Socket_Open (U16 *p, U16 Length);
int SOCKET4_HandleIP_Socket_Update (U16 *p, U16 Length);
int SOCKET4_HandleIP_Socket_Close (U16 *p, U16 Length);

void SOCKET6_free_entries(void);
void SOCKET6_delete_route(PSock6Entry pSocket);

int SOCKET6_HandleIP_Socket_Open (U16 *p, U16 Length);
int SOCKET6_HandleIP_Socket_Update (U16 *p, U16 Length);
int SOCKET6_HandleIP_Socket_Close (U16 *p, U16 Length);




