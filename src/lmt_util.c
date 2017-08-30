#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lmt_log.h"
 
static void __execp_signal_handler(int sig)
{
    void *array[100];
	char **strings; 
    int  size;  
    int i, j; 


  	lmt_log(LMT_LOG_ERROR, "Expection SIGSEGV(%d)\n",sig);  
		
    size = backtrace (array, 100); 
	
	lmt_log(LMT_LOG_ERROR, "Call Backtrace returned symbols nums: %d\n",size);  

    strings =(char **) backtrace_symbols (array, size);  
	if(strings == NULL)
	{
		lmt_log(LMT_LOG_ERROR, "Call backtrace_symbols returned NULL\n");  
	}
	else
	{	
	    for (i = 0; i < size; i++) 
		{  
	        lmt_log(LMT_LOG_ERROR, "%d %s \n",i,strings[i]);  
	    }  
		
		free (strings);  
	}

	exit(0);
      
}

static int __filesize(int fd)
{
	int leng = -1;;
	
	if((leng = lseek(fd, 0, SEEK_END)) < 0)
	{
		return -1;
	} 

	return leng;
}

Public int lmt_file_read(char *file, char buff[], int maxlen)
{
	int fd = -1;
	int fileLen = 0;
	
	if(!file)
	{
		return -1;
	}

	if((fd = open(file, O_RDONLY)) < 0)
	{
		return -2;
	}
	
	fileLen = __filesize(fd);
	if(fileLen > maxlen)
	{
		close(fd);
		return -3;
	}

	lseek(fd, 0, SEEK_SET);
	
	if(read(fd, buff, fileLen) < 0) 
	{
		close(fd);
		return -4;
	}

	close(fd);

	buff[fileLen] = 0;

	return fileLen;

}



Public int lmt_utils_init()
{
	signal(SIGPIPE, __execp_signal_handler);// 13   Pipe broke
	
	signal(SIGSEGV, __execp_signal_handler); //11   Invalid memory reference 

	signal(SIGABRT, __execp_signal_handler); // 6Core Abort signal from 

	return 0;
}

Public int lmt_process_id()
{
	static int pid = 0;

	if(pid == 0)  pid = getpid();

	return pid;
}


