CFLAGS  := $(EXTRA_CFLAGS)

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

TARGETS=udpSendTos udpRecvTos

all: $(TARGETS)

udpSendTos: udpSendTos.c
	$(CC) $(CFLAGS) -lpthread udpSendTos.c -o udpSendTos
udpRecvTos: udpRecvTos.c
	$(CC) $(CFLAGS) udpRecvTos.c -o udpRecvTos
