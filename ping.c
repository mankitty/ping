#include "ping.h"

#define MAX_WAIT_TIME		5
#define MAX_NO_PACKETS		3 
#define PACKET_SIZE		4096

int nsend = 0;
int sockfd = -1;
int datalen = 56;
int nreceived = 0;

struct sockaddr_in dest_addr;
struct timeval tvrecv; 

void print_ping_end()
{
	printf("\n--------------------PING statistics-------------------\n"); 
	printf("%d packets transmitted, %d received , %%%.2f lost\n",nsend,nreceived,(float)(nsend - nreceived)/nsend * 100); 
	close(sockfd);
	exit(1);
}

void finish_ping(int signal) 
{
	switch(signal)
	{
		case SIGINT:
			print_ping_end();
			break;
		default:
			break;
	}
} 

unsigned short cal_chksum(unsigned short *addr,int len) 
{ 
	int sum = 0; 
	int nleft = len; 

	unsigned short *w = addr; 
	unsigned short answer = 0; 

	while(nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	if(nleft == 1)
	{
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
} 

void tv_sub(struct timeval *out,struct timeval *in) 
{
	if((out->tv_usec -= in->tv_usec) < 0) 
	{ 
		--out->tv_sec; 
		out->tv_usec += 1000000; 
	}
	out->tv_sec -= in->tv_sec; 
}

int create_pack(int pack_no,pid_t self_pid,char *snbuf)
{ 
	int packsize;
	struct timeval *tval = NULL;

	struct icmp *icmp = (struct icmp *)snbuf; 
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0; 
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = pack_no;
	icmp->icmp_id = self_pid;
	packsize = sizeof(struct icmp) + datalen;
	tval = (struct timeval *)icmp->icmp_data;
	gettimeofday(tval,NULL);
	icmp->icmp_cksum = cal_chksum((unsigned short *)icmp,packsize);
	return packsize;
} 

void send_packet(pid_t self_pid) 
{
	char snbuf[PACKET_SIZE] = {'\0'};
	
	nsend++; 
	int packetsize = create_pack(nsend,self_pid,snbuf);
	if(sendto(sockfd,snbuf,packetsize,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr)) < 0)
		err_quit("sendto error");
} 

int parsing_ping_package(char *buf,int len,pid_t self_pid,struct sockaddr_in from) 
{ 
    int i,iphdrlen; 
    struct ip *ip; 
    struct icmp *icmp; 
    struct timeval *tvsend; 
    double rtt; 
    ip = (struct ip *)buf; 
    iphdrlen = ip->ip_hl << 2;
    icmp = (struct icmp *)(buf + iphdrlen);
    len -= iphdrlen;
    
    if(len < sizeof(struct icmp))
    { 
      err_msg("ICMP packets\'s length is less than icmp header\n"); 
      return -1; 
    } 
    
    if( (icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == self_pid) ) 
    { 
        tvsend=(struct timeval *)icmp->icmp_data;
        tv_sub(&tvrecv,tvsend);
        rtt = tvrecv.tv_sec * 1000 + tvrecv.tv_usec/1000.0;
        printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n", len,inet_ntoa(from.sin_addr),icmp->icmp_seq,ip->ip_ttl,rtt); 
        return 0;
    } 
    return -1; 
} 


void recv_packet(pid_t self_pid) 
{ 
	int n,fromlen; 
	extern int errno;
	char rcvebuf[PACKET_SIZE];
	fromlen = sizeof(struct sockaddr_in);
	struct sockaddr_in from;
	
	while(1)
	{
		memset(rcvebuf,0,sizeof(rcvebuf));
		if((n = recvfrom(sockfd,rcvebuf,sizeof(rcvebuf),0,(struct sockaddr *)&from,(socklen_t *)&fromlen)) < 0) 
		{ 
			if(errno == EINTR) continue; 
			err_msg("recvfrom error"); 
			continue;
		} 
		gettimeofday(&tvrecv,NULL);
		if(parsing_ping_package(rcvebuf,n,self_pid,from) == -1) continue; 
		nreceived++; 
		break;
	}
}

int main(int argc,char *argv[]) 
{
	pid_t self_pid;
	struct timeval      tv;
	int recv_size = 4 * 1024;
	struct hostent *host; 
	struct protoent *protocol; 
	unsigned long inaddr = 0l; 
	int waittime = MAX_WAIT_TIME;


	if (argc < 2)
		err_sys("usage:%s hostname/IP address",argv[0]);

	if((protocol = getprotobyname("icmp")) == NULL)
		err_sys("getprotobyname get icmp proto failed");

	if((sockfd = socket(AF_INET,SOCK_RAW,protocol->p_proto)) < 0)
		err_sys("create socket failed");

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1)
		err_sys("setsockopt set send time out failed");
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
		err_sys("setsockopt set recv time out failed");

	if (setuid(getuid()) == -1)
		err_sys("setuid failed for user");

	if (setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&recv_size,sizeof(recv_size)) == -1)
		err_sys("setsockopt recvbuf failed");

	if((self_pid = getpid()) == -1)
		err_sys("get self pid failed");

	bzero(&dest_addr,sizeof(dest_addr)); 
	dest_addr.sin_family = AF_INET;

	if((inaddr = inet_addr(argv[1])) == INADDR_NONE) 
	{ 
		if((host=gethostbyname(argv[1])) == NULL)
			err_sys("gethostbyname failed");
		memcpy( (char *)&dest_addr.sin_addr.s_addr,host->h_addr,host->h_length); 
		printf("PING %s (%s): %d bytes data in ICMP packets.\n",host->h_name, 
		inet_ntoa(dest_addr.sin_addr),datalen); 
	}
	else
	{
		memcpy((char *)&dest_addr.sin_addr.s_addr,(char *)&inaddr,sizeof(inaddr)); 
		printf("PING %s (%s): %d bytes data in ICMP packets.\n",argv[1], inet_ntoa(dest_addr.sin_addr),datalen); 
	}

	signal(SIGINT, finish_ping);

	while(1)
	{
		send_packet(self_pid);
		recv_packet(self_pid);
		sleep(1);
	}

	return 0; 

} 

