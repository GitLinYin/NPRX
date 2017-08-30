
#include <string.h>

/***************************CHUNKED CONN******************************
Chunked-Body = *chunk
	last-chunk
	trailer
	CRLF

chunk = chunk-size [ chunk-extension ] CRLF
	chunk-data CRLF
	
chunk-size =1*HEX

last-chunk = 1*(¡°0¡å) [ chunk-extension ] CRLF
	
chunk-extension= *( ¡°;¡± chunk-ext-name [ "=" chunk-ext-val ] )

chunk-ext-name = token

chunk-ext-val = token | quoted-string

chunk-data =chunk-size(OCTET)
trailer = *(entity-header CRLF) 

*********************************************************************/

static int __hex_to_dec(char c)
{
	if(c >= '0' && c <= '9')
	{
		return (int)(c - '0');
	}

	if(c >= 'a' && c <= 'f')
	{
		return (int)(c - 'a' + 10);
	}

	if(c >= 'A' && c <= 'F')
	{
		return (int)(c - 'A' + 10);
	}

	return -1;
}

static int __get_end_len(const char *buf, int buflen)
{
	const char	*s, *e;
	int		len = -1;

	for (s = buf, e = s + buflen - 1; s < e; s++)
	{
		if (s[0] == '\r' && s[1] == '\n')
		{
			len = s - buf + 2;
		}
	}

	return (len);
}


static int __get_con_len(const char *buf, int buflen, int *con)
{
	const char	*s, *e;
	int	len = 0;
	int ext = 0;
	int ret;

	if(buflen < 2)
	{
		return -1;
	}

	for (s = buf, e = buf + buflen - 1; s < e; s++)
	{
		if (s[0] == '\r' && s[1] == '\n')
		{
			goto suc;
		}

		if(ext == 1)
		{
			continue;
		}

		ret = __hex_to_dec(*s);
		if(ret < 0)
		{
			ext = 1;
			continue;
		}

		len = len *16 + ret;
		
	}

	return -1;

suc:

	*con = len;

	return (s - buf + 2);
}



int HttpChunkParse(char *c, int l)
{
	int ret = 0;
	int len = 0;
	int ofset = 0;
	
	while(1)
	{
		if((ret  = __get_con_len(c + ofset, l - ofset, &len)) < 0)
		{
			return -1;
		}

		ofset += ret; ofset += len;

		if(len == 0)
		{
			if((ret = __get_end_len(c + ofset, l - ofset)) < 0)	
			{
				return -1;
			}

			ofset += ret;

			return ofset;
		}

		if(len > 0)
		{
			if((ret = __get_end_len(c + ofset, 2)) < 0)	
			{
				return -1;
			}

			ofset += ret;
		}
		
	}


	return -1;

}



