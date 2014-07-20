/****************************************************************************//**\file
 * This file contains global variables, types, and helper functions
 * asscessible to all modules in the project.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"

// Side A and B hardware options and pins. These are used to populate the 'PongConfig'
// structure from the command line options passed to the program
#define VIDEO_A                 "/dev/video0"
#define STEPPER_DIR_PIN_A       66
#define STEPPER_STEP_PIN_A      69
#define KICKER_PIN_A            45
#define LIMIT_PIN_A             23
#define LASER_PIN_A             47

#define VIDEO_B                 "/dev/video1"
#define STEPPER_DIR_PIN_B       67
#define STEPPER_STEP_PIN_B      68
#define KICKER_PIN_B            44
#define LIMIT_PIN_B             26
#define LASER_PIN_B             46


// Name of MQs
#define MQ_NAME_VIDEO_FULL    "/MQ_NAME_VIDEO_FULL"
#define MQ_NAME_VIDEO_EMPTY   "/MQ_NAME_VIDEO_EMPTY"
#define MQ_NAME_GREEN_FULL    "/MQ_NAME_GREEN_FULL"
#define MQ_NAME_GREEN_EMPTY   "/MQ_NAME_GREEN_EMPTY"
#define MQ_NAME_RED_FULL      "/MQ_NAME_RED_FULL"
#define MQ_NAME_RED_EMPTY     "/MQ_NAME_RED_EMPTY"
#define MQ_NAME_XYCOORDS      "/MQ_NAME_XYCOORDS"

// Holds configuration of pong, filled from command-line options in main.c
PongInstance_t pongInstances[2];        //!< Contains configuration settings for the running pong program

// Global flag used to tell all threads when to stop working
volatile bool keepWorking = false;  //!< Global flag, threads will continue to work a long this value is true


/****************************************************************************//**
 * Configure the 'pongInstance' global structure based on what side of the
 * table, side A or B is used by this program. This function populates the
 * MQ names and GPIO pins fields.this function must be called before any of
 * the setupXXX() functions listed below in this module!
 * @param usingSideA  true indicate side A, false indicate side B.
 ****************************************************************************/
void configurePongInstances(void)
{
    //    Collect all hardware configuration for this instance of pong based on command line options.
    //    During the setup phase, the application will look up the values in the 'pongInstance'
    //    structure to configure the application environment.
    pongInstances[0].videoDeviceName = VIDEO_A;
    pongInstances[0].dirPin = STEPPER_DIR_PIN_A;
    pongInstances[0].stepPin = STEPPER_STEP_PIN_A;
    pongInstances[0].kickerPin = KICKER_PIN_A;
    pongInstances[0].limitSwitchPin = LIMIT_PIN_A;
    pongInstances[0].laserSensorPin = LASER_PIN_A;
    pongInstances[0].actuatorSignal = SIGUSR1;
    pongInstances[0].id = 0;
    pongInstances[0].mqNameVideoFull = MQ_NAME_VIDEO_FULL;
    pongInstances[0].mqNameVideoEmpty = MQ_NAME_VIDEO_EMPTY;
    pongInstances[0].mqNameGreenFull = MQ_NAME_GREEN_FULL;
    pongInstances[0].mqNameGreenEmpty = MQ_NAME_GREEN_EMPTY;
    pongInstances[0].mqNameRedFull = MQ_NAME_RED_FULL;
    pongInstances[0].mqNameRedEmpty = MQ_NAME_RED_EMPTY;
    pongInstances[0].mqNameXYCoords = MQ_NAME_XYCOORDS;
    pongInstances[0].cameraInfo.vid_fd = -1;
    pongInstances[0].paddlePos =   10;
    pongInstances[0].predictedPos = 10;
    pongInstances[0].newData = 0;
    pongInstances[0].cameraInfo.vid_fd = -1;

    log( "Pong is running on side-A of the table." );

    pongInstances[1].videoDeviceName = VIDEO_B;
    pongInstances[1].dirPin = STEPPER_DIR_PIN_B;
    pongInstances[1].stepPin = STEPPER_STEP_PIN_B;
    pongInstances[1].kickerPin = KICKER_PIN_B;
    pongInstances[1].limitSwitchPin = LIMIT_PIN_B;
    pongInstances[1].laserSensorPin = LASER_PIN_B;
    pongInstances[1].actuatorSignal = SIGUSR2;
    pongInstances[1].id = 1;
    pongInstances[1].mqNameVideoFull = MQ_NAME_VIDEO_FULL "B";
    pongInstances[1].mqNameVideoEmpty = MQ_NAME_VIDEO_EMPTY "B";
    pongInstances[1].mqNameGreenFull = MQ_NAME_GREEN_FULL "B";
    pongInstances[1].mqNameGreenEmpty = MQ_NAME_GREEN_EMPTY "B";
    pongInstances[1].mqNameRedFull = MQ_NAME_RED_FULL "B";
    pongInstances[1].mqNameRedEmpty = MQ_NAME_RED_EMPTY "B";
    pongInstances[1].mqNameXYCoords = MQ_NAME_XYCOORDS "B";
    pongInstances[1].cameraInfo.vid_fd = -1;
    pongInstances[1].paddlePos =   10;
    pongInstances[1].predictedPos = 10;
    pongInstances[1].newData = 0;
    pongInstances[1].cameraInfo.vid_fd = -1;

    log( "Pong is running on side-B of the table." );
}

/*setupVideo was here*/

/****************************************************************************//**
 * Initialize the GPIO pins and ISR for this pong instance.
 * @return true if setup was completed successfully, false otherwise.
 ****************************************************************************/
bool setupIO(PongInstance_t* pongInstance)
{

    static bool firstTime = true;
    if ( firstTime && gpioInit() != 0 )
    {
        logw( "Failed to initialize GPIO library on pong #%d", pongInstance->id );
        return false;
    }
    firstTime = false;

    gpioSetPinDirection( pongInstances->dirPin, OUTPUT );
    gpioSetPinDirection( pongInstances->stepPin, OUTPUT );
    gpioSetPinDirection( pongInstances->kickerPin, OUTPUT );
    gpioSetPinDirection( pongInstances->limitSwitchPin, INPUT );
    gpioSetPinDirection( pongInstances->laserSensorPin, INPUT );

    gpioWritePin( pongInstances->kickerPin, LOW );

    logw( "GPIO Initialize on pong #%d", pongInstance->id );
    return true;
}


/****************************************************************************//**
 * Initialize the processing chain queues.
 * @return true if setup was completed successfully, false otherwise.
 ****************************************************************************/
bool setupQueues(PongInstance_t* pongInstance)
{

    struct mq_attr attr;
    attr.mq_maxmsg = VFRAME_MQ_SIZE;
    attr.mq_msgsize = sizeof(int);
    attr.mq_flags = 0;

    // 1. Create queues for video frames
    if ( (pongInstance->mqVideoFull = mq_open( pongInstance->mqNameVideoFull, O_RDWR | O_CREAT, 0777, &attr )) == (mqd_t)-1 )
    {
        perror( "mqVideoFull - mq_open failed" );
        return false;
    }

    if ( (pongInstance->mqVideoEmpty = mq_open( pongInstance->mqNameVideoEmpty, O_RDWR | O_CREAT, 0777, &attr )) == (mqd_t)-1 )
    {
        perror( "mqVideoEmpty - mq_open failed" );
        return false;
    }

    // 2. Create queues for video filters
    attr.mq_maxmsg = VFILTER_MQ_SIZE;

    if ( (pongInstance->mqGreenFull = mq_open( pongInstance->mqNameGreenFull , O_RDWR | O_CREAT, 0777, &attr )) == (mqd_t)-1 )
    {
        perror( "mqGreenFull - mq_open failed" );
        return false;
    }

    if ( (pongInstance->mqGreenEmpty = mq_open( pongInstance->mqNameGreenEmpty, O_RDWR | O_CREAT, 0777, &attr )) == (mqd_t)-1 )
    {
        perror( "mqGreenEmpty - mq_open failed" );
        return false;
    }

    if ( (pongInstance->mqRedFull = mq_open( pongInstance->mqNameRedFull, O_RDWR | O_CREAT, 0777, &attr )) == (mqd_t)-1 )
    {
        perror( "mqRedFull - mq_open failed" );
        return false;
    }

    if ( (pongInstance->mqRedEmpty = mq_open( pongInstance->mqNameRedEmpty, O_RDWR | O_CREAT, 0777, &attr )) == (mqd_t)-1 )
    {
        perror( "mqRedEmpty - mq_open failed" );
        return false;
    }


    // 3. Create queues for centroid coordinates
    attr.mq_maxmsg = XYCOORDS_MQ_SIZE;
    attr.mq_msgsize = sizeof(Coords_t);

    if ( (pongInstance->mqXYCoords = mq_open( pongInstance->mqNameXYCoords, O_RDWR | O_CREAT, 0777, &attr )) == (mqd_t)-1 )
    {
        perror( "mqXYCoords - mq_open failed" );
        return false;
    }


    // 4. Put all the available frames and filter index in their
    //    respective empty queues
    int i;
    for (i=0; i < VFRAME_MQ_SIZE; i++ )
    {
        if ( mq_send( pongInstance->mqVideoEmpty, (char*)&i, sizeof(int), 0 ) == -1 )
        {
            perror( "mqVideoEmpty - mq_send" );
            return false;
        }

    }/*for*/


    for (i=0; i < VFILTER_MQ_SIZE; i++ )
    {
        if ( mq_send( pongInstance->mqGreenEmpty, (char*)&i, sizeof(int), 0 ) == -1 )
        {
            perror( "mqGreenEmpty - mq_send" );
            return false;
        }

        if ( mq_send( pongInstance->mqRedEmpty, (char*)&i, sizeof(int), 0 ) == -1 )
        {
            perror( "mqRedEmpty - mq_send" );
            return false;
        }

    }/*for*/

    return true;
}

/****************************************************************************//**
 * Stop the processing chain gracefully when the user press Ctrl-C.
 * This purges the message queues when the program exits.
 * @param sig Signal number. For this handler always SIGINT
 ****************************************************************************/
static void ctrlCHandler(int sig)
{
    stopPong(pongInstances); // Stop processing chain, stopPong() is full of system calls and
                // should not be called from a sig handler, however, we are exiting the
                // program next, so whatever breaks in the main program is irrelevant.
    exit(0);
}


/****************************************************************************//**
 * Initialize the global variable environment required to run the pong threads.
 * This function must be called before starting any thread.
 * @return true if setup was completed successfully, false otherwise.
 ****************************************************************************/
bool setupPong(PongInstance_t* pongInstance)
{
    // 1. Set a handler for CTRL-C
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR)
    {
        perror("ctrlCHandler signal failed");
        return false;
    }

    // 2. Initialize GPIO
    if ( !setupIO(pongInstance) )
    {
        perror( "Failed to setup IO" );
        return false;
    }

    // 3. Initialize video
    if ( !setupVideo(pongInstance->videoDeviceName, VFRAME_WIDTH, VFRAME_HEIGHT, &pongInstance->cameraInfo) )
    {
        perror( "Failed to setup Video" );
        return false;
    }

    // 4. Initialize MQs
    if ( !setupQueues(pongInstance) )
    {
        perror( "Failed to setup Queues" );
        return false;
    }


    // 5. No thread should be working right now
    keepWorking = false;

    log("setupPong() completed.");
    return true;
}


/****************************************************************************//**
 * Start all pong threads.
 * @return true if threads started successfully, false otherwise.
 ****************************************************************************/
bool startPong(PongInstance_t* pongInstance)
{
    // If pong already started, leave...
    /*if ( keepWorking )
    {
        logw("Called startPong() twice. Stop pong first.");
        return false;
    }*/

    keepWorking = true;

    // Reset all thread identifiers
    pongInstance->videocapThreadId        = 0;
    pongInstance->redFilterThreadId       = 0;
    pongInstance->centroidThreadId        = 0;
    pongInstance->trajectoryThreadId      = 0;
    pongInstance->greenFilterThreadId     = 0;
    pongInstance->paddleTrackerThreadId   = 0;
    pongInstance->actuatorThreadId        = 0;
    pongInstance->kickerThreadId          = 0;

    // Launch all treads
    if ( !startVideocap(pongInstance) )     return false;
    if ( !startRedFilter(pongInstance) )    return false;
    if ( !startCentroid(pongInstance) )     return false;
    if ( !startTrajectory(pongInstance) )   return false;
    if ( !startGreenFilter(pongInstance) )  return false;
    if ( !startPaddleTracker(pongInstance)) return false;
    if ( !startActuator(pongInstance) )     return false;
    if ( !startKicker(pongInstance) )       return false;


    log("startPong() completed.");
    return true;
}

/****************************************************************************//**//**
 * Set termination condition and wait for all threads to exit gracefully.
 ****************************************************************************/
void stopPong(PongInstance_t* pongInstance)
{
    // 1. Tell all thread to exit
    keepWorking = false;

    // 2. Wait for threads to end
    if ( pongInstance->videocapThreadId!= 0 )      pthread_join( pongInstance->videocapThreadId, NULL );
    if ( pongInstance->redFilterThreadId!= 0 )     pthread_join( pongInstance->redFilterThreadId, NULL );
    if ( pongInstance->centroidThreadId!= 0 )      pthread_join( pongInstance->centroidThreadId, NULL );
    if ( pongInstance->trajectoryThreadId!= 0 )    pthread_join( pongInstance->trajectoryThreadId, NULL );
    if ( pongInstance->greenFilterThreadId!= 0 )   pthread_join( pongInstance->greenFilterThreadId, NULL );
    if ( pongInstance->paddleTrackerThreadId!= 0 ) pthread_join( pongInstance->paddleTrackerThreadId, NULL );
    if ( pongInstance->actuatorThreadId!= 0 )      pthread_join( pongInstance->actuatorThreadId, NULL );
    if ( pongInstance->kickerThreadId!= 0 )        pthread_join( pongInstance->kickerThreadId, NULL );

    // 3-> Reset thread IDs
    pongInstance->videocapThreadId        = 0;
    pongInstance->redFilterThreadId       = 0;
    pongInstance->centroidThreadId        = 0;
    pongInstance->trajectoryThreadId      = 0;
    pongInstance->greenFilterThreadId     = 0;
    pongInstance->paddleTrackerThreadId   = 0;
    pongInstance->actuatorThreadId        = 0;
    pongInstance->kickerThreadId          = 0;

    // 4-> Release queues, so the don't live on after the application ends
    mq_unlink( pongInstance->mqNameVideoFull );
    mq_unlink( pongInstance->mqNameVideoEmpty );
    mq_unlink( pongInstance->mqNameGreenFull );
    mq_unlink( pongInstance->mqNameGreenEmpty );
    mq_unlink( pongInstance->mqNameRedFull );
    mq_unlink( pongInstance->mqNameRedEmpty );
    mq_unlink( pongInstance->mqNameXYCoords );

    if( closeVideo(&pongInstance->cameraInfo) == 0 )
        logw("No video device opened");
    else
        logw("Video device closed");

    gpioWritePin( pongInstance->stepPin, LOW);
    log("stopPong() completed.");
}

