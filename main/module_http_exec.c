
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

#include "lmt_log.h"
#include "lmt_net.h"
#include "lmt_ioctl.h"

#include "module_http.h"

typedef struct {
	char *vArgv;
	char *szPacket;
	int iPacketLen;
} ST_HTTP_TUNNEL_ARGV;


#define HTTP_READ_BUFF_SIZE  1024*20

#define __goto_exit(status, info) do {\
	__init_http_res_packet(szBuff, status, info, NULL, NULL, 0);\
	goto res;\
}while(0)


static int __fielter_resources(char *resources)
{
	return 0;
}

static int __init_http_res_packet(char *szResBuff, int iStatus, char *szStatusInfo, char *szHead, char *szBody, int len)
{
 	int l = 0;
	int rlen = 0;
	
	rlen += sprintf(szResBuff, "%s %d %s\r\n", "HTTP/1.1", iStatus, szStatusInfo);

	rlen += sprintf(szResBuff + rlen, "Content-Length: %d\r\n", len);

	if((szHead != NULL) && ((l = strlen(szHead)) > 1)) 
	{
		if(strstr(szHead, "\r\nConnection:") == NULL)
		{
			rlen += sprintf(szResBuff + rlen, "%s\r\n", "Connection: close");	
		}

		if((l > 3) && (strcmp(szHead + l - 4, "\r\n\r\n") == 0))// \r\n\r\n
		{
			l -= 2; szHead[l] = '\0'; 	//del last "\r\n"
		}


		if((l > 1) && (strcmp(szHead + l - 2, "\r\n") == 0))// \r\n
		{
			rlen += sprintf(szResBuff + rlen, "%s",szHead);
		}
		else
		{
			rlen += sprintf(szResBuff + rlen, "%s\r\n",szHead);
		}	
	}
	else 
	{
		rlen += sprintf(szResBuff + rlen, "%s\r\n", "Connection: close");
		rlen += sprintf(szResBuff + rlen, "%s\r\n", "Content-Type: text/xml");
	}

	 rlen += sprintf(szResBuff + rlen, "\r\n%s", szBody==NULL?"":szBody);

	return rlen;

}

static __send_to_server_callback(int sfd, int dfd, void *argv)
{
	ST_HTTP_TUNNEL_ARGV *HD = (ST_HTTP_TUNNEL_ARGV *)argv;
	int en = *((int *)HD->vArgv);
	
	if(lmt_net_m_send(dfd, HD->szPacket, HD->iPacketLen, en))
	{
		return -1;
	}

	return 0;
}

Public int Module_Http_Exec(int sfd, void *argv)
{
	int tims = 0;
	int ret = -1, len = 0;
	char * pstExp = NULL;
	ST_HTTP_PACKET stHttpPacket;
	unsigned char szBuff[HTTP_READ_BUFF_SIZE];
	ST_HTTP_TUNNEL_ARGV stHD;
	char szResources[1024];

	memset(szBuff, 0, sizeof(szBuff)); 
	
	do {
		
		if(tims++ > 3) 
		{
			lmt_net_tcp_close(sfd);
			lmt_log(LMT_LOG_ERROR, "Module_Http call lmt_net_read more [%d] times. read-buff:\n%s\n", tims, szBuff);
			return -1;
		}
		
		ret = lmt_net_read(sfd, 10, 0, szBuff + len , HTTP_READ_BUFF_SIZE - len);
		if(ret < 1)
		{
			lmt_net_tcp_close(sfd);
			lmt_log(LMT_LOG_ERROR, "Module_Http call lmt_net_read error. read-buff:\n%s\n", szBuff);
			return -1;	
		}

		len += ret;  
	
		memset(&stHttpPacket, 0, sizeof(stHttpPacket));
		ret = HttpMessageParse(&stHttpPacket, HTTP_REQ_PACKET, szBuff, len);
		if(ret == HTTP_RET_PACKET_ERR)
		{
			lmt_net_tcp_close(sfd);
			lmt_log(LMT_LOG_ERROR,"HTTP packet error(no-http msg). read-buff:\n%s\n", szBuff);
			return -1;
		}
	}while(ret < 0); //INC


	if(strcasecmp("GET", stHttpPacket.un_start_line_para1.szReqMet) == 0) //GET
	{
		//nothing
	}
	else if(strcasecmp("POST", stHttpPacket.un_start_line_para1.szReqMet) == 0)//POST
	{
		//nothing 
	}
	else
	{
		lmt_log(LMT_LOG_ERROR,"HTTP no support mode [%s], packet msg:\n%s\n:", stHttpPacket.un_start_line_para1.szReqMet, szBuff);
		__goto_exit(405, "Method Not Allowed");
	}

	pstExp = strstr(stHttpPacket.un_start_line_para2.szReqUrl, "?");
	if(pstExp == NULL)
	{
		sprintf(szResources, "%s", stHttpPacket.un_start_line_para2.szReqUrl);
	}
	else
	{
		memset(szResources, 0, sizeof(szResources));
		ret = pstExp - stHttpPacket.un_start_line_para2.szReqUrl;
		memcpy(szResources, stHttpPacket.un_start_line_para2.szReqUrl, ret);
	}

	if(szResources[0] != '/')
	{
		lmt_log(LMT_LOG_ERROR,"HTTP resources no-support: %s\n:", szResources);
		__goto_exit(403, "Forbidden");
	}

	//Call resources  Fielter fun....
	if(__fielter_resources(szResources))
	{
		__goto_exit(403, "Forbidden");
	}

	lmt_log(LMT_LOG_INFO,"Http-start-line:[%s %s]\n", stHttpPacket.un_start_line_para1.szReqMet, stHttpPacket.un_start_line_para2.szReqUrl);

	stHD.szPacket = szBuff; stHD.iPacketLen = len; stHD.vArgv = argv;

	LT_TUNNEL_Bridge(sfd, __send_to_server_callback, &stHD, *((int *)argv)); 


	return 0;

res:
	lmt_net_send(sfd, szBuff, strlen(szBuff));

	lmt_thread_sleep(2, 0);	

	lmt_net_tcp_close(sfd);
	
	return -1;

}


