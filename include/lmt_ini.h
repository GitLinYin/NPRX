#ifndef __LMT_INI_H
#define __LMT_INI_H
#include "lmt_util.h"

#define INI_SECTION_NAME_LEN  64

typedef struct __ini_section
{
	char szSectionName[INI_SECTION_NAME_LEN];
} ST_INI_SEC_ZONE; 


typedef struct __ini_hd
{
	char * name;
	int nums;
	struct __ini_section *zones;
} ST_INI_OPT_HD; 


Public ST_INI_OPT_HD * lmt_ini_hd_new(char *name);

Public int lmt_ini_hd_destory(ST_INI_OPT_HD *hd);

Public int lmt_ini_hd_get_sec_zones(ST_INI_OPT_HD *hd,  ST_INI_SEC_ZONE zones[], int maxs);

Public int lmt_ini_hd_get_sec_zone(ST_INI_OPT_HD *hd,  ST_INI_SEC_ZONE *zone, char *name);


#endif


