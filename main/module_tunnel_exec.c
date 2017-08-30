#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

#include "lmt_log.h"
#include "lmt_net.h"
#include "lmt_ioctl.h"

typedef struct __tunnel_hd
{
	int isenc;
	char *protocol;
} ST_TUNNEL_HD;

static char __szAcl[1024] = {0};

static int __Tunnel_Conn_Acl(char *ip, unsigned short port)
{
	if(__szAcl[0]  == '\0') return 0;

	if(strstr(__szAcl, ip) != NULL)
	{

		return 0;
	}

	lmt_log(LMT_LOG_WARN, "IP [%s] acl Fielter no-pass: white list ip: %s \n",ip,  __szAcl); 
}

static int __tunnel_server_runn(int sfd, void *argv)
{
	static int __work_pth_nums = 0;
	ST_TUNNEL_HD *pstHd = (ST_TUNNEL_HD *)argv;
	
	lmt_log(LMT_LOG_INFO, "Current running work-pthread nums:%d\n", ++__work_pth_nums); 

	if(pstHd->protocol == NULL)
	{
		return LT_TUNNEL_Bridge(sfd, NULL, argv, pstHd->isenc);  //TCP Tunnel
	}
	else if(strcasecmp(pstHd->protocol, "http") == 0)
	{
		Module_Http_Exec(sfd, &pstHd->isenc); //HTTP Tunnel
	}
	else if(strcasecmp(pstHd->protocol, "socks5") == 0)
	{
		Module_Socks5_Client(sfd, NULL);
	}

	__work_pth_nums--;
	
	return 0;
}


Public int Module_Tunnel(unsigned short liport, char *aclip, int isencrypt, unsigned short leport, char *protocol, char *aclfile)
{
	int sfd = -1;
	ST_TUNNEL_HD stHd;

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

	if(LT_TUNNEL_Server(liport, aclip))
	{
		return -1;
	}

	if((sfd = lmt_net_svr_tcp_create(leport)) < 0) 
	{
		return -2;
	}

	
	stHd.isenc = isencrypt; stHd.protocol = protocol;

	return lmt_net_svr_tcp_runn(sfd, &stHd, __tunnel_server_runn, 0, __Tunnel_Conn_Acl);
}



