#ifndef NO_LMT_THREAD
#include "lmt_thread.h"
#include <string.h>

typedef struct __thread_inner_argv
{
	thread_runn_callback user_callback;
	void * user_argv;
}st_thread_inner_argv;

Private void *__thread_start_run(void *argv)
{
	st_thread_inner_argv *inner_argv = (st_thread_inner_argv *)argv;

	inner_argv->user_callback(inner_argv->user_argv);

	lmt_heap_free(argv);

	pthread_exit(NULL);

	lmt_thread_sleep(1, 0); //stop 1s

	return NULL;
}
Public int lmt_thread_id()
{
	return syscall(SYS_gettid);  
}


Public int lmt_thread_sleep(int s, int us)
{
	struct timeval tv;

	if((s == 0) && (us == 0)) return 0;
	
    tv.tv_sec   = s; tv.tv_usec = us;

   	if(select(1, NULL, NULL,  NULL,  &tv)) return -1;

	return 0;
}

Public int lmt_thread_attr_default_init(pthread_attr_t *attr)
{	
	memset((char *)attr, 0, sizeof(pthread_attr_t));
	
	pthread_attr_init(attr); 
	
	pthread_attr_setstacksize(attr, 1024 * 1024);
	
	pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);

	return 0;
}


Public int lmt_thread_create(thread_attr_init_callback init_fun, thread_runn_callback run, void *argv)
{
	int ret = -1;
	pthread_t pthid;
	pthread_attr_t attr;
	st_thread_inner_argv *inner_argv = NULL;

	inner_argv =(st_thread_inner_argv *)lmt_heap_calloc(sizeof(st_thread_inner_argv));

	if(inner_argv == NULL) return -1;
	
	inner_argv->user_argv = argv;
	inner_argv->user_callback = run;
	
	if(init_fun == NULL) init_fun = lmt_thread_attr_default_init;
	
	init_fun(&attr);

	ret = pthread_create(&pthid, &attr,  __thread_start_run,  (void *)inner_argv);

	if(ret) lmt_heap_free(inner_argv);
	
	pthread_attr_destroy(&attr);

	return ret;	
}




#endif

