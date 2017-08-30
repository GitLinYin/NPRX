#ifndef __LMT_NET_H
#define __LMT_NET_H
#include "lmt_util.h"
#include <sys/types.h>			
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>


typedef int ( *net_cli_acl_callback)(char *ip, unsigned short port);
typedef int ( *net_cli_run_callback)(int sfd, void *argv);

Public int lmt_net_svr_tcp_create(unsigned short lisn);

Public int lmt_net_svr_tcp_runn(int sfd, void *argv, net_cli_run_callback run, int isBlock, net_cli_acl_callback acl);

Public int lmt_net_cli_tcp_create(char *ip, unsigned short port);

Public int lmt_net_url2ip(char *hostname, char *ip);

Public int lmt_net_read(int nfd, int t, int us, char *s, int l);

Public int lmt_net_fixed_length_read(int nfd, int t, int us, char *s, int fixlen);

Public int lmt_net_send(int fd,char *s, int l);

Public int lmt_net_bridge(int fd1, int fd2, int count, int fd1_en_mode);

Public int lmt_net_m_send(int sfd,  char *s, int len, int en_mode);

Public int lmt_net_m_recv(int sfd, unsigned char *s, int maxlen, int en_mode);

#define lmt_net_tcp_close(s)  close(s)

#endif

