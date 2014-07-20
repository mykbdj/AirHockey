/****************************************************************************//**\file
 * Utility Library
 * This library contains general purpose functions.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"
#include "time.h"     //for clock_gettime() and timespec


/****************************************************************************//**
 * Reads from a queue (received). If the queue is empty, this function blocks
 * up to a given timeout period.
 * @param mq          Queue to read from
 * @param buf         Buffer where to read the msg
 * @param size        Size of the buffer
 * @param timeout_us  Maximum time to block on a receive (usec.).
 * @return OK if the receive was successful, TIMEOUT if no msg was received
 *         in the given timeout, ERROR is there was a receive error.
 ****************************************************************************/
int mq_read( mqd_t mq, void *buf, int size, int timeout_us)
{
    struct timespec now;
    clock_gettime( CLOCK_REALTIME, &now );

    now.tv_nsec = now.tv_nsec + timeout_us * 1000L;
    if (now.tv_nsec >= 1000000000L)
    {
        now.tv_sec++;
        now.tv_nsec = now.tv_nsec - 1000000000L;
    }

    ssize_t result = mq_timedreceive( mq, (char*)buf, size, NULL, &now );
    return result == size ? OK : ( errno==ETIMEDOUT ? TIMEOUT : ERROR );
}

/****************************************************************************//**
 * Writes from a queue (received). If the queue is empty, this function blocks
 * up to a given timeout period.
 * @param mq          Queue to write to
 * @param buf         Buffer containing the msg to write
 * @param size        Size of the buffer
 * @param timeout_us  Maximum time to block on a send (usec.).
 * @return OK if the send was successful, TIMEOUT if no msg was sent
 *         in the given timeout, ERROR is there was a receive error.
 ****************************************************************************/
int mq_write( mqd_t mq, void *buf, int size, int timeout_us)
{
    struct timespec now;
    clock_gettime( CLOCK_REALTIME, &now );

    now.tv_nsec = now.tv_nsec + timeout_us * 1000L;
    if (now.tv_nsec >= 1000000000L)
    {
        now.tv_sec++;
        now.tv_nsec = now.tv_nsec - 1000000000L;
    }

    if ( mq_timedsend( mq, (char*)buf, size, 0, &now ) == -1 )
        return errno==ETIMEDOUT ? TIMEOUT : ERROR;
    else
        return OK;
}

/*xioctl*/

void xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

/**
 * Profiling stuff is here
 * startProfile stopProfile getTimeDiff
 */
struct timespec startTime;
struct timespec stopTime;

struct timeval first;
struct timeval second;

struct timespec timeDiff(struct timespec start,struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

void startProfile(void)
{
    if(clock_gettime(CLOCK_MONOTONIC_RAW, &startTime) < 0)
        loge("Start time");
}

void stopProfile(void)
{
    if(clock_gettime(CLOCK_MONOTONIC_RAW, &stopTime)<0)
        loge("Stop time");
}

void getProfile(char *threadName)
{
    loge("Profile %s %lu sec %lu millisec\n", threadName, timeDiff(startTime,stopTime).tv_sec, (timeDiff(startTime,stopTime).tv_nsec)/1000000);
}

/****************************************************************************//**
   Clears the screen.
 ****************************************************************************/
void cls()
{
    printf("\e[2J\e[1;1H");
    fflush( stdout );
}

