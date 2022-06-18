#include "log.h"
#include<stdio.h>
#include<time.h>
#include<pthread.h>
#include <unistd.h>

FILE *logger_fd;
int logger;

void * log_flush(void *arg)
{
	while (1)
	{
	     fflush(logger_fd);
	     sleep(1);   
	}
	return NULL;
}

void log_init(char *filename, int level)
{

    logger = level;
   logger_fd = fopen(filename, "a+");

   pthread_t log_thread;
    pthread_create(&log_thread, NULL, log_flush, NULL);
    pthread_detach(log_thread);
}

void log_close()
{
    fclose(logger_fd);
}
