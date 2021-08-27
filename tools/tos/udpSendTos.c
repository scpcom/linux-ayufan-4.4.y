/* udpsend.c */

#include <stdlib.h>

#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <string.h>
#include <netdb.h>
#include <pthread.h>

#include <signal.h>
#define NUM_THREADS 2

char *ipAddress;
int numeberOfPackets;
int packetSize1;
int packetSize2;

pthread_t threads[NUM_THREADS];

#define MAX_BUF 1024

char buf1[9000];

int sendUDP(char *ipAddress, int settosInt)
{
    int i, j, n, s, len;

    char *buf = "start";
    char *hostname;
    char *command[1024];

    struct servent *sp;
    struct sockaddr_in name;
    struct sockaddr_in from;

    struct hostent *hp;
	int state = 1;
    printf("Sending...\n");
    fflush(stdout);
	hp = gethostbyname(ipAddress);
    if (hp == NULL) {
		fprintf(stderr, "%s: Unknown host\n", hostname);
		exit(0);
		}
	s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
		perror("socket");
		exit(0);
		}
	if (setsockopt(s, IPPROTO_IP, IP_TOS,
				(char *)&settosInt, sizeof(int)) < 0) {
			perror(" error setting QOS sockopts");
	}

   memset(&name, 0, sizeof(struct sockaddr_in));
   name.sin_family = AF_INET;
   name.sin_port = htons(9999);
   memcpy(&name.sin_addr, hp->h_addr_list[0], hp->h_length);
   for (j = 0; j < numeberOfPackets; j++)
	   sendto(s, buf1, packetSize1, 0, (struct sockaddr *) &name, sizeof(struct sockaddr_in));

    if (n < 0) {
       perror("sendTO");
       exit(1);
       }
  close(s);
}

void *sendThread1(void *arg)
{
   int *tos_ptr;
   int tos;

   tos_ptr = (int *) arg;
   tos = *tos_ptr;

	printf("tos = %d in %s\n", tos, __func__);
	sendUDP(ipAddress, tos);
}

void *sendThread2(void *arg)
{
   int *tos_ptr;
   int tos;

   tos_ptr = (int *) arg;
   tos = *tos_ptr;

	printf("tos = %d in %s\n", tos, __func__);
	sendUDP(ipAddress, tos);
}

int main(int argc, char **argv)
{
	char *settos1;
	char *settos2;
	char *packetSize1Ptr1;
	char *packetSize1Ptr2;
	char *numeberOfPacketsPtr;
	int j, t;
	int res;
	int *tos[NUM_THREADS];
    tos[0] = (int *) malloc(sizeof(int));
	tos[1] = (int *) malloc(sizeof(int));

	for (j = 0; j < 180; j++)
		/* 50 bytes*/
		strcat(buf1, "0123456789ABCDEFGHIJKLMNOPQRS000000000000000000000");

	if (argc < 7) {
		printf("usage: udpsend ipAddress numeberOfPackets tos1 packetSize1 tos2 packetSize1\n");
		exit(0);
		}

	ipAddress = *++argv;
	numeberOfPacketsPtr = *++argv;
	numeberOfPackets = atoi(numeberOfPacketsPtr);
	settos1    = *++argv;

	packetSize1Ptr1 = *++argv;
	packetSize1 = atoi(packetSize1Ptr1);
	if (packetSize1 >= 9000) {
		printf("packetSize2 must be lower then 9000! exiting\n");
		exit(0);
	}
	settos2    = *++argv;
	packetSize1Ptr2 = *++argv;;
	packetSize2 = atoi(packetSize1Ptr2);
	if (packetSize2 >= 9000) {
		printf("packetSize2 must be lower then 9000! exiting\n");
		exit(0);
	}

	*tos[0] = atoi(settos1);
	*tos[1] = atoi(settos2);
	printf("sending with tos1=%d , tos2=%d %s\n", *tos[0], *tos[1], __func__);
    res = pthread_create(&threads[0], NULL, sendThread1, (void *) tos[0]);

    if (res) {
      printf("ERROR; return code from pthread_create() sendThread1 is %d\n", res);
      exit(-1);
      }

    res = pthread_create(&threads[1], NULL, sendThread2, (void *) tos[1]);

    if (res) {
      printf("ERROR; return code from pthread_create() sendThread2 is %d\n", res);
      exit(-1);
      }

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
	printf("%d packets were sent with tos=%d and %d packets were sent with tos=%d \n", numeberOfPackets, *tos[0],
			numeberOfPackets, *tos[1]);

}
