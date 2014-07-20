/****************************************************************************//**\file
 * Logging Utility Library
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "logger.h"
#include "string.h"
#include "errno.h"
#include "mqueue.h"
#include "fcntl.h"
#include "pthread.h"
#include "sys/time.h"
#include "stdarg.h"

/*Filter message queue variables*/
#define LOG_FILTER_MQ   "/logger_mq"      /*id of the message queue*/
#define LOG_MSG_SIZE    256               /*max message size*/
#define LOG_QUEUE_SIZE  100               /*filter queue size */
static mqd_t            logFilterMQ;
static pthread_t        logFilterTaskId = 0;

/* Specifies the current log level: 0=info/verbose, 1=warn, 2=err, 3=none*/
static LogLevel_t currentLogLevel = LOG_INFO;

/*variables used filter storage and state*/
static int  filterCount = 0;
static char filterTokens[LOG_MAX_FILTERS][LOG_FILTER_MAX_CHARS];
static char filterTmp[ LOG_MAX_FILTERS * LOG_FILTER_MAX_CHARS ];

/*forwards*/
static int  logMatchFilter( char * str );
static void *logFilterTask( void * arg );
static void logInitEx( void );


/****************************************************************************//**
   Sets the logging level.
   @param logLevel  The minimum log level required for a message to be logged.
                    Possible values are: LOG_LOW, LOG_MED, LOG_HIGH, LOG_NONE.
 ****************************************************************************/
void logSetLevel(LogLevel_t logLevel )
{
    currentLogLevel = logLevel;
}

/****************************************************************************//**
   Logs a message using a given logging level at the given x,y coordinates.
   The whole log message is limited to 256 characters (including prefix
   metadata: time, filename, line_no, etc.). This function consumes about
   512 bytes of call stack space.

   The x,y values can be used to position text anywhere in the screen. The
   top left cornet of the screen is at (1,1). If positioning is not desired
   set the X,Y values to (-1,-1) and the text is printed normally scrolling.

   @param x         Optional X coordinate
   @param y         Optional Y coordinate
   @param logLevel  Level at which the message is logged
   @param filename  Name of the source file where the message originated from.
   @param lineNo    Line number of where the message originated at
   @param fmt       Printf-like formatted string
   @param ...       Zero or more arguments ureferenced from the format string.
 ****************************************************************************/
void logMsgEx(int x, int y, LogLevel_t logLevel, char *filename, int lineNo, char *fmt, ... )
{
    va_list     ap;
    char        tmp[LOG_MSG_SIZE];
    char        msg[LOG_MSG_SIZE];
    char        *levelStr = logLevel==LOG_ERROR ? "\e[1;31m" : (logLevel==LOG_WARN ? "\e[1;34m" : "\e[0m");


    /* Discard messaged with log level below the current log level*/
    if ( logLevel < currentLogLevel  )
        return;

    /*Lazy load the logFilterTask. A performance hit is taken only on the first log post */
    if ( logFilterTaskId == 0)
        logInitEx();

    /* Generate timestamp */
    struct tm time_tm;
    struct timeval now;

    gettimeofday(&now, NULL);
    localtime_r( &now.tv_sec, &time_tm );
    strftime( tmp, sizeof(tmp), "%H:%M:%S", &time_tm );


    /* Remove path prefix from filename */
    char *pathSeparator = strrchr(filename, '/');
    if ( pathSeparator != NULL )
        filename = pathSeparator+1;

    /* Generate log message prefix */
    if ( x>0 && y>0 )
        snprintf(msg, sizeof(msg), "\e[%d;%dH[%s%s.%05ld] %s(%d) %s\e[0m", y, x, levelStr, tmp, now.tv_usec/10, filename, lineNo, fmt );
    else
        snprintf(msg, sizeof(msg), "[%s%s.%05ld] %s(%d) %s\e[0m", levelStr, tmp, now.tv_usec/10, filename, lineNo, fmt );


    /* Append user content*/
    va_start( ap, fmt );
    vsnprintf( tmp, sizeof(tmp), msg, ap );
    va_end( ap );

    /* Pass message to log filtering task*/
    if (  mq_send(logFilterMQ, tmp, LOG_MSG_SIZE, 0) == -1 )
    {
        perror( "logMsgEx - mq_send" );
    }

}

/****************************************************************************//**
   Adds a filtering to the log output. When filters are set, only log entries
   that match one or more filter tokens are output. Log entries not matching
   any of the filters are suppressed. This operation is similar to following
   the logfile from a unix shell using a grep command like this:
           tail -f logfile | grep -E "pattern1|pattern2|pattern3"

   Only a maximum of LOG_MAX_FILTERS filters can be specified, and each filter
   pattern can be upto a maximum of LOG_FILTER_MAX_CHARS characters or
   truncation will occur. This function is not thread safe. External
   synchronization must be implemented if more than one thread will call this
   function.

   Example:
       logSetFilter( "error|critical|1000|MED|HIGH|Navigation" );

   @param filters   A string containing one or more token separated by pipe "|".
                    If this value is NULL, the log filter is cleared and all
                    log messages are passed through. Note that filter patterns
                    are case-sensitive.
 ****************************************************************************/
void logSetFilter( char * filters )
{
    filterCount = 0;

    /*if filter is null, leave clearing the filters*/
    if (filters==NULL)
        return;

    /*do a local copy, as the tockenizer is destructive*/

    strncpy( filterTmp, filters, sizeof(filterTmp)-1 );


    char * token = strtok( filterTmp, "|" );
    while ( token != NULL && filterCount < LOG_MAX_FILTERS )
    {
        strncpy(filterTokens[filterCount], token, LOG_FILTER_MAX_CHARS-1 );
        token = strtok( NULL, "|" );
        filterCount ++;
    }

}

/****************************************************************************//**
   Test if a string contains any of the current filter patterns.
   @param str   A string to scan for filter pattern
   @return 1 if the given string contains any of the filter patterns
           specified with logSetFilter(); returns 0 otherwise.
 ****************************************************************************/
static int logMatchFilter( char * str )
{
    int i;

    /*Scan each filter token in the given string; Note that filter pattern
      matching is case-sensitive.*/
    if ( str!=NULL && str[0]!=0)
    {
        for ( i=0; i < filterCount; i++ )
        {
            if ( strstr( str, filterTokens[i] ) != NULL )
                return 1;
        }
    }

    return 0;
}

/****************************************************************************//**
   Starts the logFilterTask, this function is called from the first time a log
   posting is made;
 ****************************************************************************/
static void logInitEx( void )
{
    if ( logFilterTaskId != 0 )
        return;

    // setup common message q attributes
    struct mq_attr attr;
    attr.mq_maxmsg = LOG_QUEUE_SIZE;
    attr.mq_msgsize = LOG_MSG_SIZE;
    attr.mq_flags = 0;

    // create filter message queue
    logFilterMQ = mq_open( LOG_FILTER_MQ, O_RDWR | O_CREAT, 0777, &attr );

    if ( logFilterMQ == (mqd_t) -1 )
    {
        perror( "logFilterMQ - mq_open failed" );
        exit(-2);
    }

    // span filter task
    if( pthread_create( &logFilterTaskId, NULL, logFilterTask, NULL ) != 0 )
    {
        perror("Failed to create logFilterTask");
        exit(-2);
    }



}

/****************************************************************************//**
   Log filter task - This task performs filtering logic on behalf of the
   logMsgEx() and its associated macros logh(), logm(), and log(). Each log
   entry passed in the filter message queue is tested and if it passed the
   filtering criteria, the log entry is send to the output.
 ****************************************************************************/
static void *logFilterTask( void *arg )
{
    char strmsg[LOG_MSG_SIZE];
    int  count;

    /*read messages and logg'em*/
    while (1)
    {
        if ( (count = mq_receive( logFilterMQ, strmsg, LOG_MSG_SIZE, NULL )) == -1 )
        {
            perror( "logFilterTask - mq_receive" );
        }
        else
        {
            /* Pass message to undeylying log system */
            if ( filterCount==0  || logMatchFilter(strmsg) )
                puts( strmsg );
        }

    }/*while*/

    return NULL;
}



