/****************************************************************************//**\file
 * Contains the VideoCap thread business logic implementation.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"



/****************************************************************************//**
 * Video capture thread main entry point.
 * @param arg Optional thread parameter, this is unused.
 * @return Always null
 ****************************************************************************/
static void *videocapThread( void *arg )
{
    PongInstance_t* pongInstance = (PongInstance_t *) arg;

    int vIndex = -1;
    int result;

    while ( keepWorking )
    {
        // 1. Get an empty vframe buffer,
        //    only if we sucessfully sent the previous one
        if ( vIndex == -1 )
        {
            result = mq_read( pongInstance->mqVideoEmpty, &vIndex, sizeof(int), 100000 );
            if ( result!=OK )
            {
                if ( result == ERROR )
                    loge("mq_read failed - %s", strerror(errno) );

                continue;

            } /*if*/

            log("Got empty video frame index %d         ", vIndex );
            //usleep(1000000);

        }/*if*/
        #ifdef PROFILING_ON
            START_PROFILE();
        #endif
        // 2. fill vframe buffer with data from camera
        // copy data to 'pongInstance.videoFrames[vIndex]'
        getFrame((unsigned char *) &pongInstance->videoFrames[vIndex], &pongInstance->cameraInfo);

        #ifdef PROFILING_ON
            STOP_PROFILE();
            GET_PROFILE("getFrame");
        #endif

        // 3. Send vframe to the color filters threads
        result = mq_write( pongInstance->mqVideoFull, &vIndex, sizeof(int), 100000 );
        if ( result!=OK )
        {
            if ( result == ERROR )
                loge("mq_write failed - %s", strerror(errno) );

            continue;

        } /*if*/

        log("Sent full video frame index %d       ", vIndex );
        //usleep(1000000);


        vIndex = -1; //allow another receive to happen in the next iteration

    }/*while*/


    log( FG13 "  *** VideoCap exited...");
    return NULL;
}

/****************************************************************************//**
 * Starts the videocap thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startVideocap(PongInstance_t* pongInstance)
{
    if( pthread_create( &pongInstance->videocapThreadId, NULL, videocapThread, pongInstance ) != 0 )
    {
        loge("Failed to create videocap thread - %s", strerror(errno) );
        return false;
    }

    return true;
}
