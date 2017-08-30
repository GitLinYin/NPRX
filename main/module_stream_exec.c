#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

#include "lmt_log.h"
#include "lmt_net.h"
#include "lmt_ioctl.h"

typedef struct __stream_hd
{
	int isenc;
	char *szConnIP;
	unsigned short sConnPort;
} ST_STREAM_HD;


static char __szAcl[1024] = {0};

static int __Stream_Conn_Acl(char *ip, unsigned short port)
{
	if(__szAcl[0]  == '\0') return 0;

	if(strstr(__szAcl, ip) != NULL)
	{
		return 0;
	}

	lmt_log(LMT_LOG_WARN, "Ip [%s] aCL Fielter no-pass: white list ip: %s \n", ip, __szAcl); 

	return -1;
}

static int __Stream_Exec(int sfd, void *argv)
{
	int cfd = -1;
	ST_STREAM_HD * pstHD = (ST_STREAM_HD *) argv;
	
	
	cfd = lmt_net_cli_tcp_create(pstHD->szConnIP , pstHD->sConnPort);
	if(cfd < 0)
	{
		lmt_log(LMT_LOG_ERROR, "Call lmt_net_cli_tcp_create(%s, %d) return error\n", pstHD->szConnIP , pstHD->sConnPort); 
		goto __end;
	}

	lmt_net_bridge(sfd, cfd, 2, pstHD->isenc);
		
__end:

	if(cfd > 0) lmt_net_tcp_close(cfd);
	
	if(sfd > 0) lmt_net_tcp_close(sfd);

	return 0;

	
}


Public int Module_Stream_Direct(unsigned short lsport, char *aclfile, int isencrypt, char * cip, unsigned short cport)
{
	int sfd = -1;
	ST_STREAM_HD stHD;
	
	stHD.isenc = isencrypt;  stHD.szConnIP = cip; stHD.sConnPort= cport;
	
	if((sfd = lmt_net_svr_tcp_create(lsport)) < 0)
	{
		lmt_log(LMT_LOG_ERROR, "Call lmt_net_svr_tcp_create(%d,...) fail\n", lsport); 
		return -1;
	}

	if(aclfile != NULL)
	{
		lmt_log(LMT_LOG_INFO, "Init acl rule....\n"); 
		
		if(lmt_file_read(aclfile, __szAcl, sizeof(__szAcl) - 1) < 0)
		{
			lmt_log(LMT_LOG_WARN, "Read acl file [%s] error\n", aclfile); 
			__szAcl[0] = 0;
		}
		else
		{
			lmt_log(LMT_LOG_WARN, "Init acl rule success , white list ips: %s\n", __szAcl); 
		}
	}

	lmt_net_svr_tcp_runn(sfd, &stHD, __Stream_Exec, 0, __Stream_Conn_Acl);
	
	return 0;

}



Public int Module_Stream_Tunnel(char * bip, unsigned short bport, int isencrypt, char * cip, unsigned short cport)
{
	ST_STREAM_HD stHD;

	lmt_log(LMT_LOG_INFO, "Module_Stream_Tunnel:[%d-%d] [%d] [%s-%d]\n", bip, bport, isencrypt, cip, cport); 

	
	stHD.isenc = isencrypt;  stHD.szConnIP = cip; stHD.sConnPort= cport;

	return LT_TUNNEL_Client(bip, bport, &stHD, __Stream_Exec) ;
}




