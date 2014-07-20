/****************************************************************************//**\file
 * This file contains a centralized interface/facade of the pong core API.
 * This API is devided in sections and each section's implementation is
 * deffered to a separate c file.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#define  _GNU_SOURCE   // enable support for pthread_sigqueue() in "signal.h" -needs to be before #includes
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "errno.h"
#include "math.h"
#include "signal.h"
#include "pthread.h"
#include "mqueue.h"
#include "fcntl.h"
#include "logger.h"
#include "util.h"
#include "filters.h"
#include "gpio.h"
#include "video.h"

//profiling
#include "time.h"

#ifndef PONG_H        // include gatekeeper
#define PONG_H

/****************************************************************************//**
  Global Definitions
 ****************************************************************************/
#define __need_timespec            //Instruct compiler to use timespec structure

#define VFRAME_WIDTH        320
#define VFRAME_HEIGHT       240
#define VFRAME_MQ_SIZE      2
#define VFILTER_MQ_SIZE     1
#define XYCOORDS_MQ_SIZE    2

#define PUCK_SCAN_HT_START  25
#define PADDLE_SCAN_HT_END  25

#define VIDEO_DIVIDE        8000

#define START_PROFILE startProfile
#define STOP_PROFILE stopProfile
#define GET_PROFILE getProfile

//#define PROFILING_ON

/****************************************************************************//**
  Global Types
 ****************************************************************************/
// Unlike <stdbool.h>, this enum provides better
// syntax highlighting for 'true' and 'false' in eclipse :-P
typedef enum bool_t {
    false = 0,
    true = 1
} bool;

// Byte type
typedef unsigned char byte;

// Type for RGB24 video frames
typedef struct VideoFrame {
    byte data[ VFRAME_WIDTH * VFRAME_HEIGHT * 3 ]; //3 colors RGB24
} VideoFrame_t;

// Type for filtered video frame
typedef struct FilteredFrame {
    byte data[ VFRAME_WIDTH * VFRAME_HEIGHT ]; //one color in image
} FilteredFrame_t;

// Type for x,y coordinates
typedef struct Coords {
    int x,y;
} Coords_t;

//Typedef for the buffer allocation for the camera setup
typedef struct buffer {
        void *start;
        size_t length;
} buffers_t;

//Typedef for camera specific parameters
typedef struct vidReturnValue{
    int vid_fd;
    struct v4l2_requestbuffers vid_req;
    struct buffer *vid_buffer_loc;
    struct v4l2_buffer vid_buf;
    unsigned int vid_n_buffers;
} vidReturnValue_t;

// Hardware configuration for this instance of pong. This correspond to the
// commnad line options used to start pong, using hardware in either side A
// or B of the table. Note that message queue must have different names because
// even as different processes, pong-A and pong-B would otherwise be writing and
// reading from the same queues at OS level. Any new variable needed by the program
// that is sensitive to side-A-side-B concept should be put in this structure.
typedef struct PongInstance
{
    // Hardware Resources
    char *videoDeviceName;              //!< Video source: either VIDEO_A or VIDEO_B
    int  dirPin;                        //!< Either KICKER_PIN_A or KICKER_PIN_B
    int  stepPin;                       //!< Either LIMIT_PIN_A or LIMIT_PIN_B
    int  kickerPin;                     //!< Either KICKER_PIN_A or KICKER_PIN_B
    int  limitSwitchPin;                //!< Either LIMIT_PIN_A or LIMIT_PIN_B
    int  laserSensorPin;                //!< Either LASER_PIN_A or LASER_PIN_B
    int  actuatorSignal;                //!< Signal sent to the actuator
    int  id;                            //!< ID that specifies SIDE_A or SIDE_B

    // MQ names
    char *mqNameVideoFull;              //!< Name of MQ for video full frames
    char *mqNameVideoEmpty;             //!< Name of MQ for video empty frames
    char *mqNameGreenFull;              //!< Name of MQ for full green images
    char *mqNameGreenEmpty;             //!< Name of MQ for empty green images
    char *mqNameRedFull;                //!< Name of MQ for full red images
    char *mqNameRedEmpty;               //!< Name of MQ for empty red images
    char *mqNameXYCoords;               //!< Name of MQ for centroid-to-trajectory communication

    // IDs of all threads.
    pthread_t videocapThreadId;         //!< ID of the Video Capture thread
    pthread_t redFilterThreadId;        //!< ID of the Red Filter thread
    pthread_t centroidThreadId;         //!< ID of the Centroid thread
    pthread_t trajectoryThreadId;       //!< ID of the Trajectory thread
    pthread_t greenFilterThreadId;      //!< ID of the Green Filter thread
    pthread_t paddleTrackerThreadId;    //!< ID of Paddle Tracker the thread
    pthread_t actuatorThreadId;         //!< ID of Actuator the thread
    pthread_t kickerThreadId;           //!< ID of Kicker the thread

   // Processing chain queues
    mqd_t mqVideoFull;                  //!< msgs are an <int> index in videoFrames[]
    mqd_t mqVideoEmpty;                 //!< msgs are an <int> index in videoFrames[]
    mqd_t mqGreenFull;                  //!< msgs are an <int> index in greenFilters[]
    mqd_t mqGreenEmpty;                 //!< msgs are an <int> index in greenFilters[]
    mqd_t mqRedFull;                    //!< msgs are an <int> index in redFilters[]
    mqd_t mqRedEmpty;                   //!< msgs are an <int> index in redFilters[]
    mqd_t mqXYCoords;                   //!< msgs are a copy of a <Coords_t> structure

    // Buffers to hold for video and filtered frames
    VideoFrame_t    videoFrames[ VFRAME_MQ_SIZE ];      //!< Pre-allocated buffers for storing video frames
    FilteredFrame_t redFilters[ VFILTER_MQ_SIZE ];      //!< Pre-allocated buffers for storing red filter images
    FilteredFrame_t greenFilters[ VFILTER_MQ_SIZE ];    //!< Pre-allocated buffers for storing green filter images

    volatile int     paddlePos;
    volatile int     predictedPos;
    volatile byte    newData;

    vidReturnValue_t cameraInfo;        //!< Structures holding information from Camera A and Camera B
} PongInstance_t;



/****************************************************************************//**
   Global Variables defined in pong.c
 ****************************************************************************/
// Holds configuration of pong, filled from command-line options in main.c
extern  PongInstance_t pongInstances[2];        //!< Contains configuration settings for the running pong program

//Global flag used to tell all threads when to stop working
extern volatile bool keepWorking;          //!< Global flag, threads will continue to work a long this value is true


/****************************************************************************//**
 * Configure the 'pongInstance' global structure based on what side of the
 * table, side A or B is used by this program. This function populates the
 * MQ names and GPIO pins fields.
 * @param usingSideA  true indicate side A, false indicate side B.
 ****************************************************************************/
void configurePongInstances( void );

/****************************************************************************//**
 * Initialize the message queues.
 * This function is called when you need to set up queues used in the program.
 * @return true if setup was completed successfully, false otherwise.
 ****************************************************************************/
bool setupQueues(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Initialize the IO functions.
 * This function is called when you need to set up memory mapped IO used in the program.
 * @return true if setup was completed successfully, false otherwise.
 ****************************************************************************/
bool setupIO(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Initialize the global variable environment required to run the pong threads.
 * This function mustbe called before starting any thread.
 * @return true if setup was completed successfully, false otherwise.
 ****************************************************************************/
bool setupPong(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Start all pong threads.
 * @return true if threads started successfully, false otherwise.
 ****************************************************************************/
bool startPong(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Set termination condition and wait for all threads to exit gracefully.
 ****************************************************************************/
void stopPong(PongInstance_t* pongInstance);

//------8<-----------------------------------------------------------------------thread start functions
/****************************************************************************//**
 * Starts the actuator thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startActuator(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Starts the kicker thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startKicker(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Starts the red-filter thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startRedFilter(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Starts the centroid thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startCentroid(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Starts the trajectory thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startTrajectory(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Starts the green-filter thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startGreenFilter(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Starts the paddle-tracker thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startPaddleTracker(PongInstance_t* pongInstance);

/****************************************************************************//**
 * Starts the videocap thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startVideocap(PongInstance_t* pongInstance);

//------8<-----------------------------------------------------------------------other functions
/****************************************************************************//**
 * Set up the video device.
 * @param  *device  /dev/video0 or /dev/video1 depending on side A or side B
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool setupVideo(char *device, int wd, int ht, vidReturnValue_t *);

/****************************************************************************//**
 * Gets frame from the camera.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool getFrame(unsigned char *vid, vidReturnValue_t *vidStruct);

/****************************************************************************//**
 * Closes video device.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
int closeVideo(vidReturnValue_t *vidStruct);

/****************************************************************************//**
 * Filter image. Default: red
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool filterRedFrame(unsigned char *fullImg, unsigned char *filteredImg, int filterSize);

/****************************************************************************//**
 * Filter image. Default: red. Instead of green, red for now.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool filterGreenFrame(unsigned char *fullImg, unsigned char *filteredImg);

/****************************************************************************//**
 * Filter for centroid x and y in red filtered image for puck only.
 * @param
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool puckCentroid(unsigned char *img, int wd, int ht, Coords_t *coords, int pongId);

/****************************************************************************//**
 * Filter for centroid x and y in red filtered image for paddle only.
 * @param
 * @return yCentFinal
 ****************************************************************************/
int paddleCentroid(unsigned char *img, int wd, int ht, int pongId);


#endif /*PONG_H*/
