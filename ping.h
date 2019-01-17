#ifndef _PING_H_
#define _PING_H_

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

void finish_ping(int signal);
int create_pack(int pack_no,pid_t self_pid,char *snbuf);
void send_packet(pid_t self_pid);
void recv_packet(pid_t self_pid);
int parsing_ping_package(char *buf,int len,pid_t self_pid,struct sockaddr_in from);
void tv_sub(struct timeval *out,struct timeval *in);
unsigned short cal_chksum(unsigned short *addr,int len);


extern void err_ret(const char *fmt, ...);
extern void err_sys(const char *fmt, ...);
extern void err_dump(const char *fmt, ...);
extern void err_msg(const char *fmt, ...);
extern void err_quit(const char *fmt, ...);
extern void	err_doit(int, const char *, va_list);

#endif
