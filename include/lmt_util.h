#ifndef __LMT_UTIL_H
#define __LMT_UTIL_H
#include <stdlib.h>


#define Private static
#define Public 
#define IN
#define OU


#define lmt_heap_calloc(size) calloc(1, size)
#define lmt_heap_free(h) free(h)

Public int lmt_utils_init();

Public int lmt_process_id();

Public int lmt_file_read(char *file, char buff[], int maxlen);


#endif

