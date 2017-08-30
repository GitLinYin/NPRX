#ifndef __TUNNEL_H
#define __TUNNEL_H

#define KEEP_LIVE_T   		30

typedef enum {
	e_tunnel_packet_live  	   = 'L',
	e_tunnel_packet_live_ack   = 'A',
	e_tunnel_packet_drive      = 'D',
	e_tunnel_packet_error      = 'E',
}e_tunnel_packet_type;


typedef struct __def_packet {
	char stPackCmd;
	char szBUFF[1];
}ST_TUNNEL_INN_PACKET;

typedef int (* ST_m_TUNNEL_Host_Exec_CallBack)(int fd, void *argv);

typedef int (* ST_m_TUNNEL_Host_Before_Bridge_Exec_CallBack)(int ofd, int bfd, void *argv);


/***********LT_TUNNEL_Client**********************
WARNING THE FUNCTION WILL BE BLOCKED 
***********************************************/
int LT_TUNNEL_Client(char *szSvrIp, unsigned short sSvrPort, void * pstArgv, ST_m_TUNNEL_Host_Exec_CallBack fTasker) ;

int LT_TUNNEL_Server(unsigned short isListenPort, char * szCliIp );

int LT_TUNNEL_Bridge(int sfd, ST_m_TUNNEL_Host_Before_Bridge_Exec_CallBack fun, void *argv, int enc);



#endif

