#include "lmt_log.h"
#include "lmt_net.h"
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include "lmt_ioctl.h"
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

extern int errno;

typedef struct {
	net_cli_run_callback exec_fun;
	void *exec_argv;
	int cfd;
}ST_NET_CLIENT_PTH_ARGV;

Public int lmt_net_svr_tcp_create(unsigned short lisn)
{
	int ret = 1;
	int sfd = -1;
	struct sockaddr_in addr;
	
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sfd < 0)
	{
		lmt_log(LMT_LOG_ERROR, "call socket() return %d\n", sfd);
		goto err;
	}

	addr.sin_family = AF_INET;
	
	addr.sin_port = htons(lisn);
	
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,(char *) &ret, sizeof(ret));

	if((ret = bind(sfd, (struct sockaddr *)&addr, sizeof(addr))) < 0)
	{
		lmt_log(LMT_LOG_ERROR, "call bind return [%s], listen port %d\n", strerror(errno), lisn);
		goto err;
	}

	if((ret = listen(sfd, 2000)) < 0)
	{
		lmt_log(LMT_LOG_ERROR, "Call listen error: %s\n",strerror(errno));
		goto err;
	}

	return sfd;
	

err:

	if(sfd > 0) close(sfd);
	
	return -1;


}

static int ___svr_tcp_runn_pth(void *argv)
{
	ST_NET_CLIENT_PTH_ARGV *pstArgv = (ST_NET_CLIENT_PTH_ARGV *)argv;

	pstArgv->exec_fun(pstArgv->cfd, pstArgv->exec_argv);

	free(pstArgv);

	return 0;
}

Public int lmt_net_svr_tcp_runn(int sfd, void *argv, net_cli_run_callback run, int isBlock, net_cli_acl_callback acl)
{
	int cfd = -1;
	int iRet = -1;
	unsigned int uIP = 0U;
	char szIp[16+1];
	unsigned short port = 0U;
	struct sockaddr_in addr;
	unsigned short sport = 0U;
	int scLen = sizeof(struct sockaddr);
	ST_NET_CLIENT_PTH_ARGV *pstArgv = NULL;

	getsockname(sfd, (struct sockaddr *)&addr, &scLen); sport = ntohs(addr.sin_port);

	lmt_log(LMT_LOG_INFO, "tcp_runn: listenPort[%d] accept client connect....\n", sport);
	
	while((cfd = accept(sfd, (struct sockaddr *)&addr, (int *)&scLen)) > 0) 
	{		
		uIP = ntohl(addr.sin_addr.s_addr);

		sprintf(szIp, "%d.%d.%d.%d", (uIP >> 24)&0xFF, (uIP >> 16)&0xFF, (uIP >> 8)&0xFF, uIP&0xFF);
		lmt_log(LMT_LOG_INFO, "LPort [%d] accept conn: [Client Info: %s : %d]\n",sport, szIp, ntohs(addr.sin_port));		

		if(acl && acl(szIp, ntohs(addr.sin_port)))
		{
			close(cfd);
			continue;
		}

		if(!isBlock)
		{
			pstArgv = (ST_NET_CLIENT_PTH_ARGV *)malloc(sizeof(*pstArgv));
			if(pstArgv == NULL)
			{
				lmt_log(LMT_LOG_ERROR, "Call malloc(%d) fail]\n", sizeof(*pstArgv));
				close(cfd);
				continue;
			}
			pstArgv->cfd = cfd;
			pstArgv->exec_argv = argv;
			pstArgv->exec_fun = run;
			
			if(lmt_thread_create(NULL, ___svr_tcp_runn_pth, (void *)pstArgv))
			{
				lmt_log(LMT_LOG_ERROR, "Call lmt_thread_create(___svr_tcp_runn_pth) fail]\n");
				close(cfd);
			}

			continue;
		}
	
		run(cfd, argv);
		
	}
	
	return 0;

}

Public int lmt_net_cli_tcp_create(char *ip, unsigned short port)
{
		int ret;
		int hander = -1;
		struct sockaddr_in addr;
		
		hander = socket(AF_INET, SOCK_STREAM, 0);
		if(hander < 0)
		{
			lmt_log(LMT_LOG_ERROR, "Call socket return %d]\n", hander);
			return -1;
		}
	
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = (ip != NULL)?inet_addr(ip):htonl(INADDR_ANY);

		ret = 1;
		setsockopt(hander, SOL_SOCKET, SO_REUSEADDR,(char *) &ret, sizeof(ret));
	
		if((ret = connect(hander, (struct sockaddr *)&addr, sizeof(addr))) < 0)
		{
			close(hander);
			lmt_log(LMT_LOG_ERROR, "App conn svr fail , error-info[%s],  svr addr[%s:%d]\n", strerror(errno), (ip != NULL)?ip:"ALL", port);
			return -1;
		}
	
	suc:
		
		 lmt_log(LMT_LOG_INFO, "Call LT_NetSvrHander Succ: APP, Conn Mode[%s], Bind addr[%s:%d]\n","TCP", (ip != NULL)?ip:"ALL", port);
	
		return hander;
}

Public int lmt_net_url2ip(char *hostname, char *ip)
{
	unsigned long uIP;
	struct hostent *h;
	struct in_addr *addr = NULL;

 	if((h = gethostbyname(hostname)) == NULL)
 	{
    		lmt_log(LMT_LOG_ERROR, "Call Gethostbyname(%s) Err\n", hostname);
     		return -1;
 	}

	if((addr = (struct in_addr *)(h->h_addr)) == NULL)
	{
		    lmt_log(LMT_LOG_ERROR, "Gethostbyname(%s) Addr Is Null \n", hostname);
     		return -1;
	}
	
	uIP = ntohl(addr->s_addr);
	//ntohs(addr.sin_port)

	sprintf(ip, "%ld.%ld.%ld.%ld",(uIP >> 24)&0xFF, (uIP >> 16)&0xFF, (uIP >> 8)&0xFF, uIP&0xFF);

	return 0;
}


Public int lmt_net_read(int nfd, int t, int tu, char *s, int l)
{
	int iRet = -1, currLen = 0;
	ST_IOCTL_FD arrFDs[2] = {
		[0] = {
			.fd = nfd,
			.mode =  e_fd_test_r,
			.status = 0,
		}
	};
	
	iRet = lmt_ioctl_select(t,  tu,  arrFDs,  1);
	if(iRet < 1) return iRet;

	currLen = recv(nfd, s, l, 0);
	if(currLen < 0)
	{
		lmt_log(LMT_LOG_ERROR, "Call recv fail. errCode=%d\n",currLen);
		return -1;
	}

	if(currLen == 0)
	{
		lmt_log(LMT_LOG_ERROR, "Conn be closed...\n");
		return -1;
	}
	return currLen;
}

Public int lmt_net_fixed_length_read(int nfd, int t, int us, char *s, int fixlen)
{
	struct timeval  start;
	struct timeval  end;
	long timer = 0;
	long to = t*1000000 + us;
	int len = 0;
	int ret;
	

	gettimeofday(&start, NULL);
	
	do
	{
		ret = lmt_net_read(nfd, 0, to - timer, s + len, fixlen - len);
		if(ret < 0)
		{
			return ret;;
		}

		len += ret;
		gettimeofday(&end, NULL);
			
		timer =  (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);

		if(to <= timer)
		{
			return len;
		}
			
	}while(len < fixlen);

	return len;
}

Public int lmt_net_send(int fd,char *s, int l)
{
	int count;
	int iRet = -1;
	int offset = 0;
	ST_IOCTL_FD arrFDs[1] = {
		[0] = {
			.fd = fd,
			.mode =  e_fd_test_w,
			.status = 0,
		}
	};

	if(l < 1) return 0;

	for(count = 0; count < 6; count++)
	{
		iRet = lmt_ioctl_select(10,  0,  arrFDs,  1);
		if(iRet < 0)
		{
			lmt_log(LMT_LOG_ERROR, "lmt_ioctl_select write check fail ,errCode=%d\n", iRet);
			return -1;
		}
		
		if(iRet == 0)
	    {
	    	lmt_log(LMT_LOG_ERROR, "Check Write Timeout, Conntinue...\n");
	    	continue;
	    }

		if((iRet = send(fd, s + offset, l - offset , MSG_NOSIGNAL)) <= 0)
		{
			lmt_log(LMT_LOG_ERROR, "send fail\n");
			return -1;
		}

		offset += iRet;

		if(offset == l)
		{
			return 0;
		}

		if(offset > l)
		{
			lmt_log(LMT_LOG_ERROR, "Write Error...(sumLen = %d, writeLen=%d)\n", l, offset);
			return -1;
		}
		
	}
		

	return -1;
}


static int __e_d_data(char *k, int mode, unsigned char *s, int l)
{
	int i = 0;

	//lmt_log(LMT_LOG_DEBUG, "__e_d_data....\n");

	for(i = 0; i < l; i++)
	{
		s[i] = ((s[i] & 0xF0) >> 4) + ((s[i] & 0x0F) << 4);
	}

	return 0;
}

Public int lmt_net_m_recv(int sfd, unsigned char *s, int maxlen, int en_mode)
{	
	int len = -1;

	if((len = recv(sfd, s, maxlen, 0)) < 0)
	{
		return -1;
	}

	if(len == 0) return 0;

	if(en_mode)
	{
		__e_d_data(NULL, 'd', s, len);
	}

	return len;
}

Public int lmt_net_m_send(int sfd,  char *s, int len, int en_mode)
{	
	if(en_mode)__e_d_data(NULL, 'e', s, len);
	
	lmt_net_send(sfd, s, len);

	return 0;
}

static int __er2w(int fd1, int en_mode1, int fd2,  int en_mode2)
{
	int iRet = -1;
	int iLen = -1;
	int offset = 0;
	unsigned char szBuff[1024*100];

	iLen = lmt_net_m_recv(fd1, szBuff, sizeof(szBuff), en_mode1);
	if(iLen < 0)
	{
		lmt_log(LMT_LOG_ERROR, "recv fail :%d\n", iLen);
		return -1;
	}
	
	if(iLen == 0)
	{
		lmt_log(LMT_LOG_WARN, "socket be closed\n");
		return -1;
	}

	//szBuff[iLen] = 0; lmt_log(LMT_LOG_INFO, "EGINFO[%d]:\n%s\n", iLen, szBuff);

	if(lmt_net_m_send(fd2,szBuff, iLen, en_mode2))
	{
		return -1;
	}
	
	return 0;
}


Public int lmt_net_bridge(int fd1, int fd2, int count, int fd1_en_mode)
{
	int i = 0;
	int iRet = -1;
	ST_IOCTL_FD arrFDs[2] = {
		{fd1, e_fd_test_r, 0},
		{fd2, e_fd_test_r, 0},
	};
	int times = (count < 1)?1:count;

	while(i < times)
	{	
		iRet = lmt_ioctl_select(60,  0,  arrFDs,  2);
		if(iRet < 0)
		{
			lmt_log(LMT_LOG_ERROR, "Call lmt_ioctl_select error:%d\n", iRet);
			return -1;
		}

		if(iRet == 0)
		{
			i++;
			continue;
		}

		i = 0;

		if(lmt_ioctl_status_ok(arrFDs+0) && __er2w(fd1, fd1_en_mode, fd2, 0))
		{
			return -1;
		}

		if(lmt_ioctl_status_ok(arrFDs+1) && __er2w(fd2, 0, fd1, fd1_en_mode))
		{
			return -1;
		}

	}

	return 0;

}



