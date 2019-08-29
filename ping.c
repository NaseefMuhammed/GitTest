#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 
#include <netinet/ip_icmp.h> 
#include <time.h> 
#include <fcntl.h> 
#include <signal.h> 
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PACKETSIZE	64

struct packet
{
	struct icmphdr hdr;
	char msg[PACKETSIZE-sizeof(struct icmphdr)];
};

char dstadr[40];
int pid=-1;

unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;

	return result;
}


void display(void *buf, int bytes) {

	int i;
	struct iphdr *ip = buf;
	struct icmphdr *icmp = buf+ip->ihl*4;

	printf("IPv%d: from: %s  packet size: %d protocol: %d TTL: %d ",ip->version,dstadr, ntohs(ip->tot_len), ip->protocol,ip->ttl);

	if ( icmp->un.echo.id == pid )	{
		printf("ICMP: type[%d/%d] seq[%d]\n",icmp->type, icmp->code, icmp->un.echo.sequence);

	}
}


void listener(void) {

	int sd;
	struct sockaddr_in addr;
	unsigned char buf[1024];

	if((sd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {

		perror("socket error");
		exit(0);
	}
	for (;;)
	{
		int bytes, len=sizeof(addr);
		bzero(buf, sizeof(buf));
		bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len);

		if ( bytes > 0 )
			display(buf, bytes);

		else
			perror("recvfrom");
	}
	exit(0);
}



void ping(struct sockaddr_in *addr){

	const int val=64;
	int i, sd,cnt=1;
	struct packet pckt;
	struct sockaddr_in r_addr;
	char test[64] = {"This message created by Naseef for testing a custom ping. "};

	if((sd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {

		perror("socket creation  error");
		return;
	}

	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("Set TTL option");

	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		perror("Request nonblocking I/O");

	for (;;)
	{
		int len=sizeof(r_addr);
		recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len);
		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = pid;
		for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = test[i];
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0 )
			perror("sendto");

		sleep(1);
	}
}





int main(int count, char *strings[]) {

	struct hostent *hostname;
	struct sockaddr_in addr;

	if ( count != 2 ) {
		printf("Please give the address as arguement. Format is ./ping <address>\n\t\t E.g : sudo ./ping google.com \n");
		exit(0);
	}
	if ( count == 2 ) {
		pid = getpid();
		hostname = gethostbyname(strings[1]);

		addr.sin_family = hostname->h_addrtype;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = *(long*)hostname->h_addr;
		strcpy(dstadr,strings[1]);
		if ( fork() == 0 )
			listener();

		else
			ping(&addr);
		wait(0);
	} else {
		exit(0);
	}

	return 0;

}

