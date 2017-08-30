#include <stdio.h>
#include <string.h>

#include "lmt_log.h"
#include "lmt_net.h"
#include "lmt_ioctl.h"


static int  __PrintUsageInfo()
{
	fprintf(stderr, "\n\tNPRX Tool(ah:linyinma@gmail.com): V2.0.0\n\n");	
		
	fprintf(stderr,  "Usage: nprx [select] [para1] [para2].....\n");

	fprintf(stderr,  "Call Module_Socks5_Direct:  1 lsport isencrypt aclfile\n");
	fprintf(stderr,  "Call Module_Socks5_Tunnel:  2 bip bport isencrypt\n");
	fprintf(stderr,  "Call Module_Stream_Direct:  3 lsport aclfile isencrypt cip cport\n");
	fprintf(stderr,  "Call Module_Stream_Tunnel:  4 bip bport isencrypt cip cport\n");
	fprintf(stderr,  "Call Module_Tunnel:  5 liport aclip isencrypt leport protocol aclfile\n\n");
	
	return 0;
}


#define __formt_null(s) (strcasecmp(s, "NULL") == 0)?NULL:s

int main(int argc, char **argv)
{
	int select = -1;

	lmt_utils_init();
	
	argc--; argv++;

	if(argc == 0) return __PrintUsageInfo();

	select = atoi(argv[0]); if(select > 5 || select < 1) return __PrintUsageInfo();
	
	argc--; argv++;

	if(select == 1) 
	{
		if(argc != 3) return __PrintUsageInfo();
		return Module_Socks5_Direct(atoi(argv[0]), atoi(argv[1]),	__formt_null(argv[2]));
	}
	
	if(select == 2) 
	{
		if(argc != 3) return __PrintUsageInfo();
		
		return Module_Socks5_Tunnel(argv[0], atoi(argv[1]), atoi(argv[2]));
	}

	if(select == 3)
	{
		if(argc != 5) return __PrintUsageInfo();
		
		return Module_Stream_Direct(atoi(argv[0]), __formt_null(argv[1]), atoi(argv[2]), argv[3], atoi(argv[4]));
	}

	if(select == 4)
	{
		if(argc != 5) return __PrintUsageInfo();
		
		return Module_Stream_Tunnel(argv[0], atoi(argv[1]), atoi(argv[2]), argv[3], atoi(argv[4]));
	}

	if(select == 5)
	{
		if(argc != 6) return __PrintUsageInfo();
		
		return Module_Tunnel(atoi(argv[0]),  __formt_null(argv[1]),  atoi(argv[2]), atoi(argv[3]),__formt_null(argv[4]), __formt_null(argv[5]));
	}
	
	return 0;
}

