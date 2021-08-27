/* udpRecvTos.c */

#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<stdio.h>
#include<netinet/ip_icmp.h>
#include<netinet/udp.h>
#include<netinet/tcp.h>
#include<netinet/ip.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>
#include <stdlib.h>
#include <string.h>

void ProcessPacket(unsigned char* , int);
void print_ip_header(unsigned char* , int);
void print_udp_header(unsigned char* , int);
void print_udp_packet(unsigned char *Buffer , int Size);

int tos1;
int tos2;
int tos1counter = 0;
int tos2counter = 0;

int sock_raw;
int tcp = 0, udp = 0, icmp = 0, others = 0, igmp = 0, total = 0, i, j;
struct sockaddr_in source, dest;

int main(int argc, char **argv)
{
    int saddr_size , data_size;
    struct sockaddr_in saddr;
    struct in_addr in;
	char *settos1;
	char *settos2;

    unsigned char *buffer = (unsigned char *)malloc(65536);
	if (argc < 3) {
		printf("usage: ./udpRecvTos tos1 tos2\n");
		exit(0);
		}

	settos1    = *++argv;
	settos2    = *++argv;

	tos1 = atoi(settos1);
	tos2 = atoi(settos2);

    printf("**** Starting...listening on port 9999\n");
    sock_raw = socket(AF_INET , SOCK_RAW , IPPROTO_UDP);
    if (sock_raw < 0) {
		printf("Socket Error");
		return 1;
    }

    while (1) {
		saddr_size = sizeof saddr;
		data_size = recvfrom(sock_raw , buffer , 65536 , 0 ,
							 (struct sockaddr *)&saddr,
							 &saddr_size);
		if (data_size < 0) {
			printf("read error , failed to get packetsn");
			return 1;
			}
		ProcessPacket(buffer , data_size);
		}
    close(sock_raw);
    printf("Finished");
    return 0;
}

void ProcessPacket(unsigned char *buffer, int size)
{
    struct iphdr *iph = (struct iphdr *)buffer;

    ++total;
    switch (iph->protocol) {
    case 17: /* UDP Protocol */
		++udp;
		print_udp_packet(buffer , size);
		break;
	 }
}

void print_udp_packet(unsigned char *Buffer , int Size)
{

    unsigned short iphdrlen;
	static int counter = 0;

    struct iphdr *iph = (struct iphdr *)Buffer;
    iphdrlen = iph->ihl*4;
    struct udphdr *udph = (struct udphdr *)(Buffer + iphdrlen);
	if (ntohs(udph->dest) == 9999) {
		counter++;
		if (!(counter%100))
			printf("tos1counter=%d, tos2counter=%d \n", tos1counter, tos2counter);
		if ((unsigned int)iph->tos == tos1)
			tos1counter++;
		if ((unsigned int)iph->tos == tos2)
			tos2counter++;
	}
}
