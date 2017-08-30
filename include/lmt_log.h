#ifndef __LMT_LOG_H
#define __LMT_LOG_H
#include "lmt_util.h"

#define	LMT_LOG_DEBUG 	1
#define	LMT_LOG_INFO	2
#define LMT_LOG_WARN    3
#define	LMT_LOG_ERROR   4
#define	LMT_LOG_CLOSE   5

Public int lmt_log_init(int level);

Public int lmt_log_print(int level, char *file, int line, char * fmt, ...);

#define lmt_log(l, f, ...) lmt_log_print(l, __FILE__, __LINE__, f, ##__VA_ARGS__)

#endif


