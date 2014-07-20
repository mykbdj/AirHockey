/****************************************************************************//**\file
 * Utility Library
 * This library contains general purpose functions.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/

// Return values from mq_read and mq_write
#define TIMEOUT         -2
#define ERROR           -1
#define OK               0

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
int mq_read( mqd_t mq, void *buf, int size, int timeout_us);

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
int mq_write( mqd_t mq, void *buf, int size, int timeout_us);

/*xioctl*/

void xioctl(int fh, int request, void *arg);

/*
 * Profiling stuff
 */

void startProfile(void);

void stopProfile(void);

void getProfile(char *threadName);

/****************************************************************************//**
   Clears the screen.
 ****************************************************************************/
void cls();
