/****************************************************************************//**\file
 * Logging Utility Library
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#ifndef LOGGER_H
#define  LOGGER_H


/* Colors */
#define NOC  "\e[0m"    // Default FG/BG

#define FG0  "\e[30m"   // Black
#define FG1  "\e[31m"   // Red
#define FG2  "\e[32m"   // Green
#define FG3  "\e[33m"   // Yellow
#define FG4  "\e[34m"   // Blue
#define FG5  "\e[35m"   // Magenta
#define FG6  "\e[36m"   // Cyan
#define FG7  "\e[37m"   // White
#define FG8  "\e[1;30m" // Gray
#define FG9  "\e[1;31m" // Bright Red
#define FG10 "\e[1;32m" // Bright Green
#define FG11 "\e[1;33m" // Bright Yellow
#define FG12 "\e[1;34m" // Bright Blue
#define FG13 "\e[1;35m" // Bright Magenta
#define FG14 "\e[1;36m" // Bright Cyan
#define FG15 "\e[1;37m" // Bright Cyan

#define BG0  "\e[40m"   // Black
#define BG1  "\e[41m"   // Red
#define BG2  "\e[42m"   // Green
#define BG3  "\e[43m"   // Yellow
#define BG4  "\e[44m"   // Blue
#define BG5  "\e[45m"   // Magenta
#define BG6  "\e[46m"   // Cyan
#define BG7  "\e[47m"   // White

/* Filter maximum limits */
#define LOG_MAX_FILTERS         10     /*Max filters that can be set at one given time*/
#define LOG_FILTER_MAX_CHARS    20     /*Max length in characters of each filter token*/

/* Log level constants */
typedef enum LogLevel_t {
    LOG_INFO = 0,
    LOG_WARN = 1,
    LOG_ERROR = 2,
    LOG_NONE = 3
} LogLevel_t;


/************************************************************************
   Logs a message using info logging level.
   @param fmt   Printf-like formatted string
   @param ...   Zero or more arguments ureferenced from the format sctring.
 ************************************************************************/
#define log(fmt, ...)  logMsgEx( -1,-1, LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__ )
//#define log(fmt, ...)   while(0)

/************************************************************************
   Logs a message using warning logging level.
   @param fmt   Printf-like formatted string
   @param ...   Zero or more arguments ureferenced from the format sctring.
 ************************************************************************/
//#define logw(fmt, ...)  logMsgEx( -1,-1, LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__ )
#define logw(fmt, ...)

/************************************************************************
   Logs a message using Error logging level.
   @param fmt   Printf-like formatted string
   @param ...   Zero or more arguments ureferenced from the format sctring.
 ************************************************************************/
#define loge(fmt, ...)  logMsgEx( -1,-1, LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__ )
//#define loge(fmt, ...)  while(0)

/************************************************************************
   Logs a message using info logging level.
   @param fmt   Printf-like formatted string
   @param ...   Zero or more arguments ureferenced from the format sctring.
 ************************************************************************/
//#define logxy(x,y,fmt, ...)  logMsgEx( x,y, LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__ )
#define logxy(fmt, ...)

/************************************************************************
   Logs a message using warning logging level.
   @param fmt   Printf-like formatted string
   @param ...   Zero or more arguments ureferenced from the format sctring.
 ************************************************************************/
//#define logwxy(x,y,fmt, ...)  logMsgEx( x,y, LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__ )
#define logwxy(x,y,fmt, ...)

/************************************************************************
   Logs a message using Error logging level.
   @param fmt   Printf-like formatted string
   @param ...   Zero or more arguments ureferenced from the format sctring.
 ************************************************************************/
//#define logexy(x,y,fmt, ...)  logMsgEx( x,y, LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__ )
#define logexy(x,y,fmt, ...)

/****************************************************************************//**
   Sets the logging level.
   @param logLevel  The minimum log level required for a message to be logged.
                    Possible values are: LOG_INFO, LOG_WARN, LOG_ERROR, LOG_NONE.
 ****************************************************************************/
void logSetLevel(LogLevel_t logLevel );

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
void logMsgEx(int x, int y, LogLevel_t logLevel, char *filename, int lineNo, char *fmt, ... );

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
       logSetFilter( "error|critical|1000|ERR|INFO|Navigation" );

   @param filters   A string containing one or more token separated by pipe "|".
                    If this value is NULL, the log filter is cleared and all
                    log messages are passed through. Note that filter patterns
                    are case-sensitive.
 ****************************************************************************/
void logSetFilter( char * filters );



#endif /*LOGGER_H*/
