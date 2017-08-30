
#ifndef __HTTP_H
#define __HTTP_H
 
/***************************Error Code Table*********************************/
#define HTTP_RET_PACKET_ERR   (-1) //Pack parse error
#define HTTP_RET_PACKET_INC   (-2) //Pack incomplete

/*************************************************************************/

typedef enum
{
	MSG_USE_CL = 1,  //Content-Length
	MSG_USE_TE = 2,	 //Transfer-Encoding
	MSG_USE_ER = 0,  //Error type
}ST_HTTP_MSSAGE;

typedef enum
{
	HTTP_REQ_PACKET = 0,
	HTTP_RES_PACKET = 1,
}ST_HTTP_PACKET_TYPE;

#define HTTP_HEAD_INDEX_CL 0  	//Content-Length
#define HTTP_HEAD_INDEX_TE 1	//Transfer-Encoding
#define HTTP_HEAD_INDEX_CT 2    //Content-Type
//#define HTTP_HEAD_INDEX_UA 3    //User-Agent
//#define HTTP_HEAD_INDEX_IS 4    //If-Modified-Since
//#define HTTP_HEAD_INDEX_A_ 5    //Authorization
//#define HTTP_HEAD_INDEX_R_ 6    //Referer
//#define HTTP_HEAD_INDEX_C_ 7    //Cookie
//#define HTTP_HEAD_INDEX_L_ 8    //Location
//#define HTTP_HEAD_INDEX_S_ 9    //Status
//#define HTTP_HEAD_INDEX_RA 10   //Range
//#define HTTP_HEAD_INDEX_CO 11   //Connection
//....
#define HTTP_HEAD_INDEX_EN 128 //END

typedef struct
{	
	ST_HTTP_PACKET_TYPE enPacketType;
	char *szMsg;
	
	int iMsgLen;

	union {	
		char szReqMet[64];
		char szResVer[64];
	} un_start_line_para1;

	union {	
		char szReqUrl[2048];
		char szResSta[2048];
	}  un_start_line_para2;

	int arrHeadPostion[HTTP_HEAD_INDEX_EN];
	int iHeadStart;
	int iHeadLen;
		
	ST_HTTP_MSSAGE type;
	int len; //body len
}ST_HTTP_PACKET;


/************************HttpMessageParse*********************************************
HTTP报文解析函数:
返回值:成功(大于0):返回HTTP报文长度；
			失败(小于0):当返回HTTP_RET_PACKET_INC时代表报文不完整应该继续接收
************************************************************************************/
int HttpMessageParse(ST_HTTP_PACKET *packet, ST_HTTP_PACKET_TYPE type, char *msg, int len);

#endif

