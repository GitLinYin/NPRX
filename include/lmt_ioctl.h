#ifndef __LMT_IOCTL_H
#define __LMT_IOCTL_H
#include "lmt_util.h"

typedef int ST_IOCTL_FD_STATUS;
typedef int ST_IOCTL_FD_TYPE;

typedef enum 
{
	e_fd_test_r = 0,
	e_fd_test_w = 1,
	e_fd_test_e = 2,
} e_ioctl_fd_mode;


typedef struct __ioctl_fd
{
	int fd;
	int mode; 
	int status; 
} ST_IOCTL_FD;

Public int lmt_ioctl_select(int s,  int us, ST_IOCTL_FD arrFDs[], int iFDs);

#define lmt_ioctl_status_ok(s) (((ST_IOCTL_FD *)(s))->status)

#endif

