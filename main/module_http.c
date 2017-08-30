

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

#include "module_http.h"



/****************Http Message Length***************************************
1.Any response message which "MUST NOT" include a message-body (such as the 1xx, 204, and 304 responses and any response to a HEAD request) is always terminated by the first empty line after the header fields, regardless of the entity-header fields present in the message.

2.If a Transfer-Encoding header field (section 14.41) is present and has any value other than "identity", then the transfer-length is defined by use of the "chunked" transfer-coding (section 3.6), unless the message is terminated by closing the connection.

3.If a Content-Length header field (section 14.13) is present, its decimal value in OCTETs represents both the entity-length and the transfer-length. The Content-Length header field MUST NOT be sent if these two lengths are different (i.e., if a Transfer-Encoding

     header field is present). If a message is received with both a
     Transfer-Encoding header field and a Content-Length header field,
     the latter MUST be ignored.

4.If the message uses the media type "multipart/byteranges", and the transfer-length is not otherwise specified, then this self- delimiting media type defines the transfer-length. This media type MUST NOT be used unless the sender knows that the recipient can parse it; the presence in a request of a Range header with multiple byte- range specifiers from a 1.1 client implies that the client can parse multipart/byteranges responses.

       A range header might be forwarded by a 1.0 proxy that does not
       understand multipart/byteranges; in this case the server MUST
       delimit the message using methods defined in items 1,3 or 5 of
       this section.

5.By the server closing the connection. (Closing the connection cannot be used to indicate the end of a request body, since that would leave no possibility for the server to send back a response.)

For compatibility with HTTP/1.0 applications, HTTP/1.1 requests containing a message-body MUST include a valid Content-Length header field unless the server is known to be HTTP/1.1 compliant. If a request contains a message-body and a Content-Length is not given, the server SHOULD respond with 400 (bad request) if it cannot determine the length of the message, or with 411 (length required) if it wishes to insist on receiving a valid Content-Length.

All HTTP/1.1 applications that receive entities MUST accept the "chunked" transfer-coding (section 3.6), thus allowing this mechanism to be used for messages when the message length cannot be determined in advance.

Messages MUST NOT include both a Content-Length header field and a non-identity transfer-coding. If the message does include a non- identity transfer-coding, the Content-Length MUST be ignored.

When a Content-Length is given in a message where a message-body is allowed, its field value MUST exactly match the number of OCTETs in the message-body. HTTP/1.1 user agents MUST notify the user when an invalid length is received and detected.

**************************************************************/

typedef enum {
	HDR_DATE 	= 0, 
	HDR_INT  	= 1, 
	HDR_STRING	= 2,
}ST_HEAD_TYPE;  /* HTTP header types 	   */

typedef struct vec {
	const char	*ptr;
	int 	len;
}ST_HTTP_METHOD;

typedef struct header {
	int		len;		/* Header name length		*/
	int		type;		/* Header type			*/
	const char	*name;	/* Header name			*/
}ST_HTTP_HEAD;


static const struct vec _known_http_methods[] = {
	{"GET",		3},
	{"POST",	4},
	{"PUT",		3},
	{"DELETE",	6},
	{"HEAD",	4},
	{NULL,		0}
};

/*
 * This structure tells how HTTP headers must be parsed.
 * Used by parse_headers() function.
 */
static const ST_HTTP_HEAD http_headers[] = {
[HTTP_HEAD_INDEX_CL]={16, HDR_INT,	   "Content-Length: "	},
[HTTP_HEAD_INDEX_TE]={19, HDR_STRING, "Transfer-Encoding: "	},
[HTTP_HEAD_INDEX_CT]={14, HDR_STRING, "Content-Type: "	},
	//{12, HDR_STRING, "User-Agent: "		},
	//{19, HDR_DATE,	 "If-Modified-Since: "	},
	//{15, HDR_STRING, "Authorization: "	},
	//{9,  HDR_STRING, "Referer: "		},
	//{8,  HDR_STRING, "Cookie: "		},
	//{10, HDR_STRING, "Location: "		},
	//{8,  HDR_INT,	 "Status: "		},
	//{7,  HDR_STRING, "Range: "		},
	//{12, HDR_STRING, "Connection: "		},
	{0,  HDR_INT,	 NULL			}
};

static int __get_hdr_int(char *c, int l)
{
	for(; *c == ' ' || *c == '\t'; c++); //jump space
	
	return atoi(c);
}

static char *__get_hdr_str(char *c, int l, char *g)
{
	memset(g, 0, l - 1); memcpy(g , c, l - 2);

	return g;
}


/*
 * Check whether full request is buffered Return headers length, or 0
 */
static int __get_line_len(const char *buf, int buflen)
{
	const char	*s, *e;
	
	/* Control characters are not allowed but >=128 is. */
	for (s = buf, e = s + buflen; s < e; s++)
	{
		if (!isprint(* (unsigned char *) s) && *s != '\r' && *s != '\n' && * (unsigned char *) s < 128)
		{
			return -1;
			
		}else if (s[0] == '\n')
		{
			return ( s - buf + 1);
		}
	}

	return -1;
}

static int __set_request_method(char *c, int l, ST_HTTP_PACKET *s)
{
	int i,j;
	const struct vec *v = NULL;
	
	for (v = (struct vec *)&_known_http_methods[0]; v->ptr != NULL; v++)
	{
		if (!memcmp(c , v->ptr, v->len)) 
		{
			memcpy(s->un_start_line_para1.szReqMet, v->ptr, v->len); 
			for(j= 0,i = v->len + 1 ;i < l && c[i] != ' '; i++,j++)
			{
				s->un_start_line_para2.szReqUrl[j] = c[i];
			}
			break;
		}
	}

	return ((v->ptr == NULL)?-1:0);
}

static int __set_response_method(char *c, int l, ST_HTTP_PACKET *s)
{
	int i;

	for(i = 0 ;i < l && c[i] != ' '; i++)
	{
		s->un_start_line_para1.szResVer[i] = c[i];
	}

	memcpy(s->un_start_line_para2.szResSta, c + (i +1), l - (i + 1));

	return 0;
}


static int __set_request_head(char *c, int l, ST_HTTP_PACKET *s)
{
	int i;
	const ST_HTTP_HEAD *v = NULL;

	for(i = 0, v= (ST_HTTP_HEAD *)&http_headers[0]; v->len != 0; v++, i++)
	{
		if((l > v->len) && (strncasecmp(c , v->name, v->len) == 0))
		{
			s->arrHeadPostion[i] = l + v->len;
			
			if(i == HTTP_HEAD_INDEX_CL)
			{
				s->type = MSG_USE_CL; s->len = __get_hdr_int(c + v->len, l - v->len);
			}
			else if(i == HTTP_HEAD_INDEX_TE)
			{
				s->type = MSG_USE_TE; s->len = __get_hdr_int(c + v->len, l - v->len);
			}
		}
	}

	return 0;
}


static int __http_start_line_parse(char *c, int l, ST_HTTP_PACKET *s)
{
	int msl = 0;

	if((msl = __get_line_len(c, l)) < 0) return HTTP_RET_PACKET_INC;

	if(s->enPacketType == HTTP_REQ_PACKET)
	{
		if(__set_request_method(c, msl, s) < 0)
		{
			return HTTP_RET_PACKET_ERR;
		}
	}
	else 
	{
		if(__set_response_method(c, msl, s) < 0)
		{
			return HTTP_RET_PACKET_ERR;
		}

	}


	return msl;
	
}

static int __http_message_header_parse(char *c, int l, ST_HTTP_PACKET *s)
{
	int ret = 0;
	int msl = 0;

	while(msl < l)
	{
		ret = __get_line_len(c + msl, l - msl);	
		if(ret < 0)
		{
			return HTTP_RET_PACKET_INC;
		}

		if(ret < 3) break;

		if(__set_request_head(c + msl, ret, s) < 0)
		{
			return HTTP_RET_PACKET_ERR;
		}

		msl += ret;
		
	}

	return (msl + ret);

}

static int __http_message_body_parse(char *c, int l, ST_HTTP_PACKET *s)
{
	int ret = 0;
	
	if(s->type == MSG_USE_ER)
	{
		s->len = l;
		return l;
	}

	if(s->type == MSG_USE_TE)
	{
		ret = HttpChunkParse(c, l);

		if(ret < 0) return ret;

		s->len = ret;
	}

	if(l < s->len)  //incomplete
	{
		return HTTP_RET_PACKET_INC;
	}

	if((l - s->len) <= 2) //\r\n
	{
		return l;  
	}

	return s->len;
}



int HttpMessageParse(ST_HTTP_PACKET *packet, ST_HTTP_PACKET_TYPE type, char *msg, int len)
{
	int ofset = 0;
	ST_HTTP_PACKET szPacket;
	int ret = HTTP_RET_PACKET_ERR;

	if(!msg || (len == 0))  goto err;
	
	if(packet == NULL) packet =(ST_HTTP_PACKET *) &szPacket;
	memset((char *)packet, 0, sizeof(ST_HTTP_PACKET));

	packet->enPacketType = type; packet->szMsg = msg;;

	//Parse start line
	ret = __http_start_line_parse(msg + ofset, len - ofset, packet);
	if(ret < 0) goto err; 
	ofset += ret;
	
	// Parse packet head
	ret = __http_message_header_parse(msg + ofset, len - ofset, packet);
	if(ret < 0) goto err; 
	packet->iHeadStart = ofset;     packet->iHeadLen = ret;
	ofset += ret;
	
	//Parse packet body
	ret = __http_message_body_parse(msg + ofset, len - ofset, packet);
	if(ret < 0) goto err; 
	ofset += ret;

	packet->iMsgLen = ofset;

	return ofset;

	err:

	return ret;
	
}




