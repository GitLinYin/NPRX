#include "lmt_log.h"
#include "lmt_net.h"
#include "lmt_thread.h"

#include "module_tunnel.h"

typedef struct 
{
	int iIsBlock;
	int iListenFd;
	int iClientConnFd;
	int siInnerListenPort;
	ST_m_TUNNEL_Host_Exec_CallBack execFun;
	lmt_thread_mutex_t pstClientConnFdLock;
	char szCliIp[16+1];
}ST_Server_Host_Exec_OPT;

ST_Server_Host_Exec_OPT __s_svr_conf;

int __m_TUNNEL_Server_Inner_Socket_Get(int ts)
{
	int ret = -1;
	long count = 0;
	ST_TUNNEL_INN_PACKET stPacket = {.stPackCmd = e_tunnel_packet_drive};
	
	do {	
	
		lmt_thread_mutex_lock(__s_svr_conf.pstClientConnFdLock);
		
		ret = __s_svr_conf.iClientConnFd;
		__s_svr_conf.iClientConnFd = -1;
		
		lmt_thread_mutex_unlock(__s_svr_conf.pstClientConnFdLock);
		if(ret > 0)
		{
			lmt_log(LMT_LOG_INFO, "Send tunnel bridge cmd...\n"); 
			lmt_net_send(ret, (char *)&stPacket, sizeof(stPacket));
				
			return ret;
		}

		count++;
		if(count > ts)
		{
			return ret;
		}

		lmt_thread_sleep(1, 0);//1s
		
	}while(ret < 0);

	return ret;

}

static int	__m_TUNNEL_Server_Inner_Socket_Set(int cfd, void *argv)
{
	lmt_thread_mutex_lock(__s_svr_conf.pstClientConnFdLock);
	
	if(__s_svr_conf.iClientConnFd > 0)
	{
		lmt_net_tcp_close(__s_svr_conf.iClientConnFd);
	}
	__s_svr_conf.iClientConnFd = cfd;
	
	lmt_thread_mutex_unlock(__s_svr_conf.pstClientConnFdLock);

	return 0;
}


int __m_TUNNEL_Server_Inner_Socket_Live(void *argv)
{
	ST_TUNNEL_INN_PACKET stPacket = {.stPackCmd = e_tunnel_packet_live};
	struct timeval  start;
	struct timeval  end;
	long timer;

	lmt_log(LMT_LOG_INFO, "TUNNEL Server keeplive runn...\n"); 
	
	do {	
		gettimeofday(&start, NULL);
		
		lmt_thread_mutex_lock(__s_svr_conf.pstClientConnFdLock);
		if(__s_svr_conf.iClientConnFd > 0)
		{
			lmt_log(LMT_LOG_INFO, "Send tunnel live cmd...\n"); 
			if(lmt_net_send(__s_svr_conf.iClientConnFd, (char *)&stPacket, sizeof(stPacket)))
			{
				lmt_log(LMT_LOG_ERROR, "Send live cmd error, drop it...\n"); 
				lmt_net_tcp_close(__s_svr_conf.iClientConnFd);
				__s_svr_conf.iClientConnFd = -1;
			}
		}
		else
		{
			lmt_log(LMT_LOG_INFO, "Drop tunnel live cmd...\n"); 
		}
		lmt_thread_mutex_unlock(__s_svr_conf.pstClientConnFdLock);

		gettimeofday(&end, NULL);

		timer =  (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000;
		if(KEEP_LIVE_T > timer)
		{
			lmt_thread_sleep(KEEP_LIVE_T - timer, 0);
		}
		
	}while(1);

	return 0;
}


static int  __m_TUNNEL_Server_Host_Acl_CallBack(char * ip, unsigned short port)
{
	if(__s_svr_conf.szCliIp[0]  == '\0') return 0;

	if(strcmp(ip, __s_svr_conf.szCliIp) != 0)
	{
		lmt_log(LMT_LOG_WARN, "ACL Fielter no-pass: white list ip: %s \n", __s_svr_conf.szCliIp); 

		return -1;
	}

	return 0;
}

int __m_TUNNEL_Server_Inner_Socket_Listen(void *argv)
{
	lmt_log(LMT_LOG_INFO, "TUNNEL Server listenner runn...\n"); 
	
	return lmt_net_svr_tcp_runn(__s_svr_conf.iListenFd, NULL, __s_svr_conf.execFun, 0, __m_TUNNEL_Server_Host_Acl_CallBack);
}


int LT_TUNNEL_Server(unsigned short isListenPort, char * szCliIp) 
{	
	lmt_log(LMT_LOG_INFO, "LT_TUNNEL_Server Start.... \n", isListenPort);	

	__s_svr_conf.iIsBlock = 1;
	
	__s_svr_conf.iListenFd = -1;

	if(szCliIp != NULL)
	{
		sprintf(__s_svr_conf.szCliIp, "%s", szCliIp);
	}
	else
	{
		__s_svr_conf.szCliIp[0] = '\0';
	}
	
	__s_svr_conf.siInnerListenPort = isListenPort;
	
	__s_svr_conf.execFun = __m_TUNNEL_Server_Inner_Socket_Set;
	
	lmt_thread_mutex_init(__s_svr_conf.pstClientConnFdLock);

	lmt_log(LMT_LOG_INFO, "LT_TUNNEL_Server create listen port:%d...\n",__s_svr_conf.siInnerListenPort);
	__s_svr_conf.iListenFd = lmt_net_svr_tcp_create(__s_svr_conf.siInnerListenPort);
	if(__s_svr_conf.iListenFd < 0)
	{
		goto __err;
	}
	else
	{
		lmt_log(LMT_LOG_INFO, "Call lmt_net_svr_tcp_create(%d) return:(sockfd = %d)\n", __s_svr_conf.siInnerListenPort, __s_svr_conf.iListenFd);	
	}

	lmt_log(LMT_LOG_INFO, "LT_TUNNEL_Server create listen pthread...\n");
	if(lmt_thread_create(NULL, __m_TUNNEL_Server_Inner_Socket_Listen, NULL))
	{
		goto __err;
	}

	lmt_log(LMT_LOG_INFO, "LT_TUNNEL_Server create keeplive pthread...\n");
	if(lmt_thread_create(NULL, __m_TUNNEL_Server_Inner_Socket_Live, NULL))
	{
		goto __err;
	}

	return 0;

__err:

	if(__s_svr_conf.iListenFd > 0)
	{
		lmt_net_tcp_close(__s_svr_conf.iListenFd );
	}

	return -1;
}


int LT_TUNNEL_Bridge(int sfd, ST_m_TUNNEL_Host_Before_Bridge_Exec_CallBack fun,void *argv, int enc) 
{
	int isfd = -1;
		
	if((isfd = __m_TUNNEL_Server_Inner_Socket_Get(KEEP_LIVE_T + 2)) < 0)
	{
		goto __err;
	}

	if((fun != NULL) && fun(sfd, isfd, argv))
	{
		lmt_log(LMT_LOG_WARN, "LT_TUNNEL_Bridge Exec bef-bridge fun error...\n");
		goto __err;
	}

	lmt_net_bridge( isfd, sfd, 2, enc); //2*60s

__err:
	if(sfd > 0)
	{
		lmt_net_tcp_close(sfd);
	}

	if(isfd > 0)
	{
		lmt_net_tcp_close(isfd);
	}
		
	return 0;
}


