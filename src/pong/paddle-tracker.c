/****************************************************************************//**\file
 * Contains the paddle-tracker thread business logic implementation.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"



/****************************************************************************//**
 * Paddle-tracker thread main entry point.
 * @param arg Optional thread parameter, this is unused.
 * @return Always null
 ****************************************************************************/
static void *paddleTrackerThread( void *arg )
{
    volatile PongInstance_t* pongInstance = (PongInstance_t*) arg;
    int fIndex = -1; // Input Filter buffer
    int result;
    int pos;
    //union sigval sval;

    while ( keepWorking )
    {
        // 1. Get a full filter buffer, only if we were successful releasing the previous one  (ie. fIndex == -1)
        if ( fIndex == -1 )
        {
            result = mq_read( pongInstance->mqGreenFull, &fIndex, sizeof(int), 100000 );
            if ( result != OK )
            {
                if ( result == ERROR )
                    loge( "fIndex mq_read failed - %s", strerror(errno) );

                continue;

            } /*if*/

            log( "Got full filter index %d         ", fIndex );
            //usleep(1000000);

            // 2. Process filter image, find paddle location, and reported to the actuator thread
            //    returns k as paddle 'pos'
            #ifdef PROFILING_ON
                START_PROFILE();
            #endif

            pos = paddleCentroid( (unsigned char*) &pongInstance->greenFilters[fIndex], VFRAME_WIDTH, VFRAME_HEIGHT, pongInstance->id );

            #ifdef PROFILING_ON
                STOP_PROFILE();
                GET_PROFILE("getPaddlePos");
            #endif

            // 3. Send predicted puck position value to actuator thread
          //  sval.sival_int = 0x01000000 | pos;     // The upper MSB byte=01h, tells the actuator thread what type of value
                                                   // is sent in the signal, 01h is reserved for "current paddle position"
            log( "pos is %d", pos );

//            if ( pthread_sigqueue( pongInstance->actuatorThreadId, pongInstance->actuatorSignal, sval ) == 0 )
//            {
//                log( "sent signal out current pos=%d      ", pos );
//                //This should happen!
//            }
//            else
//            {
//                loge( "pthread_sigqueue failed: %s", strerror(errno) );
//            }
            pongInstance->paddlePos = pos;

        }/*if*/

        // 4. Return used filtered image back to green filter thread
        result = mq_write( pongInstance->mqGreenEmpty, &fIndex, sizeof(int), 100000 );
        if ( result != OK )
        {
            if ( result == ERROR )
                loge( "fIndex mq_write failed - %s", strerror(errno) );

            continue;

        } /*if*/

        log( "Put back empty filter index %d       ", fIndex );
        //usleep(1000000);

        fIndex = -1; //allow another filter receive to happen in the next iteration

    }/*while*/

    log( FG13 "  *** PaddleTracker exited..." );
    return NULL ;
}

/****************************************************************************//**
 * Starts the paddle-tracker thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startPaddleTracker(PongInstance_t* pongInstance)
{
    if( pthread_create( &pongInstance->paddleTrackerThreadId, NULL, paddleTrackerThread, pongInstance ) != 0 )
    {
        loge("Failed to create paddle-tracker thread - %s", strerror(errno) );
        return false;
    }

    return true;
}
