#include "lmt_log.h"
#include "lmt_net.h"
#include "module_tunnel.h"

typedef struct {
	int iSocketFd;
	void *pstUsrArgv;
	ST_m_TUNNEL_Host_Exec_CallBack fUsrTasker;
}ST_Client_Host_Exec_OPT;

static int	__m_TUNNEL__Client_Inner_Host_Exec_CallBack(void *argv)
{
	ST_Client_Host_Exec_OPT *pstClientHEexcHD= (ST_Client_Host_Exec_OPT *)argv;

	pstClientHEexcHD->fUsrTasker(pstClientHEexcHD->iSocketFd, pstClientHEexcHD->pstUsrArgv);

	free(argv);
	
	return 0;
}

static int	__m_TUNNEL_Connect(char *szSvrIp, unsigned short sSvrPort)
{
	int sfd = -1;

	while((sfd = lmt_net_cli_tcp_create(szSvrIp, sSvrPort)) < 0)
	{
		lmt_log(LMT_LOG_ERROR, "Call lmt_net_cli_tcp_create(%s, %d), sleep 5s reconnection\n", szSvrIp, sSvrPort);
		lmt_thread_sleep(5, 0);
		continue;
	}
	
	return sfd;
}

static int	__m_TUNNEL_Keeplive(int sfd)
{
	int len = 0;
	ST_TUNNEL_INN_PACKET stPacket;

	
	while(1)
	{
		len = lmt_net_fixed_length_read(sfd, KEEP_LIVE_T + 2, 0, (char *)&stPacket, sizeof(stPacket));
		if(len != sizeof(stPacket))
		{
			lmt_log(LMT_LOG_ERROR, "__m_TUNNEL_Keeplive: recv keeplive msg timieout\n"); 	
			return -1;
		}

		if(stPacket.stPackCmd != e_tunnel_packet_live)
		{
			lmt_log(LMT_LOG_INFO, " __m_TUNNEL_Keeplive(cmd): run...\n"); 
			return 0;
		}

		lmt_log(LMT_LOG_INFO, "__m_TUNNEL_Keeplive(cmd): sleep...\n"); 
	}

	return -1;

}


static int	__m_TUNNEL_Fork(int sfd, void * pstArgv, ST_m_TUNNEL_Host_Exec_CallBack fTasker)
{
	ST_Client_Host_Exec_OPT *pstClientHEexcHD = NULL;

	if((pstClientHEexcHD = (ST_Client_Host_Exec_OPT *)malloc(sizeof(ST_Client_Host_Exec_OPT))) == NULL)
	{
		lmt_log(LMT_LOG_ERROR, "Call malloc(%d) fail\n", sizeof(ST_Client_Host_Exec_OPT));	
		return -1;	
	}

	pstClientHEexcHD->iSocketFd = sfd;  pstClientHEexcHD->pstUsrArgv = pstArgv; pstClientHEexcHD->fUsrTasker = fTasker;

	if(lmt_thread_create(NULL, __m_TUNNEL__Client_Inner_Host_Exec_CallBack, (void *)pstClientHEexcHD))
	{
		lmt_log(LMT_LOG_ERROR, "Call lmt_thread_create(...) fail\n");	
		return -1;	
	}

	return 0;

}


int  LT_TUNNEL_Client(char *szSvrIp, unsigned short sSvrPort, void * pstArgv, ST_m_TUNNEL_Host_Exec_CallBack fTasker) 
{
	int s = 0;;
	int sfd = -1;

	while(1)
	{
		lmt_log(LMT_LOG_INFO, "In fun _m_TUNNEL_Client :  TUNNEL Client Start Runn: s-%d....\n",++s); 
			
		sfd = __m_TUNNEL_Connect(szSvrIp, sSvrPort);

		if(__m_TUNNEL_Keeplive(sfd))
		{
			lmt_net_tcp_close(sfd);
			continue;
		}

		if(__m_TUNNEL_Fork(sfd, pstArgv, fTasker))
		{
			lmt_net_tcp_close(sfd);
			continue;
		}
		
	}

	return 0;
}


