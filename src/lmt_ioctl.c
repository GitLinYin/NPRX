
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "lmt_ioctl.h"

int lmt_ioctl_select(int s,  int us, ST_IOCTL_FD arrFDs[], int iFDs)
{
	int i;
	int ret;
	int max = -1;
	fd_set  fdset[3];
	struct timeval tv;

	FD_ZERO(fdset + e_fd_test_r); 
	FD_ZERO(fdset + e_fd_test_w); 
	FD_ZERO(fdset + e_fd_test_e);
	
	for(i = 0;  i < iFDs;  i++)
	{
		max = (max > arrFDs[i].fd )?max:arrFDs[i].fd;

		FD_SET(arrFDs[i].fd,  fdset + arrFDs[i].mode);

		arrFDs[i].status = 0;
	}

	tv.tv_sec   = s; tv.tv_usec = us;

	ret = select(max + 1, fdset + e_fd_test_r,  fdset + e_fd_test_w,  fdset + e_fd_test_e,  &tv);
	if(ret <= 0)
	{
		return ret;
	}
	
	for(i = 0; i < iFDs; i++)
	{
		arrFDs[i].status  =  FD_ISSET(arrFDs[i].fd,  fdset +  arrFDs[i].mode);
	}

	return ret;

}


