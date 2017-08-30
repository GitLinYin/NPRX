#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

#include "lmt_log.h"
#include "lmt_net.h"
#include "lmt_ioctl.h"


static char __szAcl[1024] = {0};

static int __e_d_data(char *k, int mode, unsigned char *s, int l)
{
	int i = 0;

	for(i = 0; i < l; i++)
	{
		s[i] = ((s[i] & 0xF0) >> 4) + ((s[i] & 0x0F) << 4);
	}

	return 0;
}


static int __cal_recv(int sfd, int t, unsigned char *s, int fixlen, int en_mode)
{	
	if(lmt_net_fixed_length_read(sfd, t, 0, s, fixlen) != fixlen)
	{
		return -1;
	}

	if(en_mode)
	{
		__e_d_data(NULL, 'd', s, fixlen);
	}

	return 0;
}


static int __cal_send(int sfd,  char *s, int len, int en_mode)
{	
	if(en_mode)__e_d_data(NULL, 'e', s, len);
	
	lmt_net_send(sfd, s, len);

	return 0;
}


static int __Socks5_Conn_Acl(char *ip, unsigned short port)
{
	if(__szAcl[0]  == '\0') return 0;

	if(strstr(__szAcl, ip) != NULL)
	{
		return 0;
	}

	lmt_log(LMT_LOG_WARN, "ACL Fielter no-pass: white list ip: %s \n", __szAcl); 

	return -1;
}


/*************************************
+----+----------+----------+
|VER | NMETHODS | METHODS  |
+----+----------+----------+
| 1  |	  1 	| 1 to 255 |
+----+----------+----------+
*************************************/
static int __Socks5_Svr_Method_Exchange(int sfd, int en_mode)
{
	unsigned char szBuff[277];
		unsigned char szEchg[2] = "\x05\x00";

	lmt_log(LMT_LOG_INFO, "Start socks5 method exChange...\n");
	
	if(__cal_recv(sfd, 2, szBuff, 3, en_mode))
	{
		lmt_log(LMT_LOG_ERROR, "Socks5 method exChange fail:recv version error\n");
		return -1;
	}

	if((szBuff[1] != 1) && __cal_recv(sfd, 3, szBuff + 3, szBuff[1] - 1, en_mode))// Method length, From the second character to start receiving
	{
		lmt_log(LMT_LOG_ERROR, "Socks5 method exChange fail:recv METHODS list error\n");
		return -2;
	}
	
	if(szBuff[0] != '\x05')
	{
		lmt_log(LMT_LOG_ERROR, "Socks5 method exChange fail: version not support:[%02X]\n", szBuff[0]);
		return -3;
	}
	
	return __cal_send(sfd, szEchg, 2, en_mode); // NO AUTHENTICATION REQUIRED
	
}


/****************************************************
+----+-----+-------+------+----------+----------+
|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
+----+-----+-------+------+----------+----------+
| 1  |	1  | X'00' |  1   | Variable |	  2 	|
+----+-----+-------+------+----------+----------+
*****************************************************/
static int __Socks5_Svr_Requests_Do(int sfd, unsigned char szIP[], unsigned short *usDst, int en_mode)
{
	int iDstLen = 0;
	int iDstType = 0;
	unsigned char sdst[1024];
	unsigned char sbuff[1024];

	lmt_log(LMT_LOG_INFO, "Start socks5 requests do...\n");
	
	if(__cal_recv(sfd, 5, sbuff, 5, en_mode))
	{
		lmt_log(LMT_LOG_ERROR, "Socks5 requests do fail:recv 1..5 chars error\n");
		return -1;
	}
	
	if(sbuff[3] == 0x01)//IP Addr
	{
		iDstType = 0;
	}
	else if(sbuff[3] == 0x03)//url
	{
		iDstType = 1;
	}
	else
	{
		lmt_log(LMT_LOG_ERROR, "Socks5 requests do fail: not support atyp:[%02X]\n", sbuff[3]);
		return -1;
	}

	iDstLen = sbuff[4];
	if(iDstLen < 1 || iDstLen > 1021)
	{
		lmt_log(LMT_LOG_ERROR, "Socks5 requests do fail: DST.ADDR len [%d] more maxlen [%d]\n", iDstLen, 1021);
		return -1;
	}

	if(__cal_recv(sfd, 5, sbuff, iDstLen + 2, en_mode))
	{
		lmt_log(LMT_LOG_ERROR, "Socks5 requests do fail: read DST.ADDR fail\n");
		return -1;	
	}
	

	memcpy(sdst, sbuff, iDstLen); sdst[iDstLen] = 0;
	
	
	*usDst= (sbuff[iDstLen]<< 8) +sbuff[iDstLen+1];

	
	lmt_log(LMT_LOG_INFO, "SOCKS5 ---> Dst-Host:[%s:%d]\n", sdst, *usDst);

	if(lmt_net_url2ip(sdst, szIP))
	{
		lmt_log(LMT_LOG_ERROR, "Call lmt_net_url2ip(%s) fail\n", sdst);
		return -1;
	}

	return 0;
}






static int __Socks5_Exec(int sfd, void *argv)
{
	int dfd = -1;
	unsigned short usDst;
	unsigned char szIP[32+1];
	int en_mode = (argv != NULL)?(*((int *)argv)):0;
	unsigned char szResInfo[10 + 1] = "\x05\x00\x00\x01\x34\x45\xf2\x5e\x1f\x41";
	static int __work_pth_nums = 0;
	
	lmt_log(LMT_LOG_INFO, "Current running work-pthread nums:%d\n", ++__work_pth_nums); 



	//STEP1:  Method_Exchange
	if(__Socks5_Svr_Method_Exchange(sfd, en_mode))
	{
		goto exit;
	}


	//STEP2:  Do Requests
	if(__Socks5_Svr_Requests_Do(sfd, szIP, &usDst, en_mode))
	{
		goto exit;
	}

	//STEP3 Resonpe Client

	szResInfo[8] = usDst >> 8;	szResInfo[9] = usDst & 0xff;	
	if(__cal_send(sfd, szResInfo, 10, en_mode))
	{
		goto exit;
	}


	//STEP4 Conn  re-Server
	dfd = lmt_net_cli_tcp_create(szIP, usDst);
	if(dfd < 0)
	{
		lmt_log(LMT_LOG_ERROR, "Call lmt_net_cli_tcp_create(%s, %d) fail\n",szIP, usDst);
		goto exit;
	}

	lmt_net_bridge(sfd, dfd, 1, en_mode);
	
exit:
	
	if(sfd > 0) lmt_net_tcp_close(sfd);

	if(dfd > 0) lmt_net_tcp_close(dfd);

	__work_pth_nums--;
	
	return 0;

}



Public int Module_Socks5_Direct(unsigned short lsport, int isencrypt,  char *aclfile)
{
	int sfd = -1;
	
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

	lmt_net_svr_tcp_runn(sfd, &isencrypt, __Socks5_Exec, 0, __Socks5_Conn_Acl);
	
	return 0;
}


Public int Module_Socks5_Tunnel(char * bip, unsigned short bport, int isencrypt)
{
	return LT_TUNNEL_Client(bip, bport, &isencrypt, __Socks5_Exec) ;
}


Public int Module_Socks5_Client(int sfd, void *argv)
{

}


