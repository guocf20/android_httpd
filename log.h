#ifndef APP_PUBLIC_LOGGER_H_
#define APP_PUBLIC_LOGGER_H_

#include<stdio.h>

extern FILE *logger_fd;
#define __DEBUG    //日志模块总开关，注释掉将关闭日志输出

#ifdef __DEBUG
    #define DEBUG(format, ...) \
	   do { \
		   if (logger_fd != NULL) \
		   fprintf(logger_fd, format, ##__VA_ARGS__);\
		   else   \
		   printf (format, ##__VA_ARGS__); \
	   }while(0)
#else
    #define DEBUG(format, ...)
#endif

//定义日志级别
enum LOG_LEVEL {    
    LOG_LEVEL_OFF=0,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_ALL
};


void log_init();

void log_close();

#define log_fatal(level,format, ...) \
    do { \
         if(level>=LOG_LEVEL_FATAL)\
           DEBUG("fatal %s:%d(%s) %s-%s " format "\n",\
                     __FILE__, __LINE__,__func__, __DATE__,__TIME__, ##__VA_ARGS__ );\
    } while (0)

#define log_err(level,format, ...) \
    do { \
         if(level>=LOG_LEVEL_ERR)\
           DEBUG("error %s:%d(%s) %s-%s " format "\n",\
                     __FILE__, __LINE__,__func__, __DATE__,__TIME__, ##__VA_ARGS__ );\
    } while (0)

#define log_warn(level,format, ...) \
    do { \
         if(level>=LOG_LEVEL_WARN)\
           DEBUG("warning %s:%d(%s) %s-%s " format "\n",\
                     __FILE__, __LINE__,__func__, __DATE__,__TIME__, ##__VA_ARGS__ );\
    } while (0)

#define log_info(level,format, ...) \
    do { \
         if(level>=LOG_LEVEL_INFO)\
           DEBUG("info %s:%d(%s) %s-%s " format "\n",\
                     __FILE__, __LINE__,__func__, __DATE__,__TIME__, ##__VA_ARGS__ );\
    } while (0)

#define log_debug(level,format, ...) \
    do { \
         if(level>=LOG_LEVEL_ALL)\
           DEBUG("\n %s:%d(%s) %s-%s " format "\n",\
                     __FILE__, __LINE__,__func__, __DATE__,__TIME__, ##__VA_ARGS__ );\
    } while (0)

#endif /* APP_PUBLIC_LOGGER_H_ */


