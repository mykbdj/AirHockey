/****************************************************************************//**\file
 * Video Library
 * This library contains video capture functions.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

#ifndef VIDEO_H        // include gatekeeper
#define VIDEO_H

#define CLEAR(x) memset(&(x), 0, sizeof(x))

//Function to write to a file when needed..can be moved to pong.h if need to call it outside video.c and video.h
int writeImage(char *fileName, int height, int width,char *bufferAddress, int numBytes);


#endif /*VIDEO_H*/
