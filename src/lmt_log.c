#include "lmt_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

//LOG BUFF MAXLEN
#define LMT_LOG_MAX_LENGTH 10240

struct __log_msg_type
{
	char *name;
	char *color;
} __log_msg_event[] =
{
	[LMT_LOG_DEBUG] = {"DEBUG",   "\033[0m" },
	[LMT_LOG_INFO] =  {"RUNNING", "\033[32m"}, 
	[LMT_LOG_WARN] =  {"WARNING", "\033[33m"},
	[LMT_LOG_ERROR] = {"ERROR",   "\033[31m"},
};

static time_t __lmt_current_time; 

static int __lmt_log_status = LMT_LOG_DEBUG;

Public int lmt_log_init(int level) {__lmt_log_status = level; 	return 0;}

Public int lmt_log_print(int level, char *file, int line, char * fmt, ...)
{
	int len = 0;
	va_list arg;
	time_t stTime = time(NULL);
	char date[64], buff[LMT_LOG_MAX_LENGTH + 1];

	if(__lmt_log_status > level) return 0;

	strftime(date, LMT_LOG_MAX_LENGTH, "%Y-%m-%d:%H:%M:%S", localtime(&stTime));

	len = sprintf(buff,"%sPID[%05d] PTH[%05d] [%s] [%-8s] [%-12s: %04d] :",__log_msg_event[level].color, lmt_process_id(), lmt_thread_id(), date,  __log_msg_event[level].name, file, line);

	va_start(arg, fmt); vsnprintf(buff + len, LMT_LOG_MAX_LENGTH - len, fmt, arg); va_end (arg);

	fprintf(stderr, "%s%s", buff, "\33[0m");

	return 0;
}



