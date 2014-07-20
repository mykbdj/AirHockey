/****************************************************************************//**\file
 * Contains the red-filter thread business logic implementation.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"



/****************************************************************************//**
 * Red-filter thread main entry point.
 * @param arg Optional thread parameter, this is unused.
 * @return Always null
 ****************************************************************************/
static void *redFilterThread( void *arg )
{
    PongInstance_t* pongInstance = (PongInstance_t*) arg;

    int vIndex = -1; // Input Video frame index
    int fIndex = -1; // Output Filter buffer
    int result;

    while ( keepWorking )
    {
        // 1. Get a full vframe buffer, only if we sucessfully released the previous one (ie. vIndex == -1)
        if ( vIndex == -1 )
        {
            result = mq_read( pongInstance->mqVideoFull, &vIndex, sizeof(int), 100000 );
            if ( result!=OK )
            {
                if ( result == ERROR )
                    loge("mq_read failed - %s", strerror(errno) );

                continue;

            } /*if*/


            log("Got full video frame index %d         ", vIndex );
            //usleep(1000000);


            // 2. Inner loop: Retrieve empty filter buffer, convert the vframe to filtered image,
            //    pass it along to the centroid thread. Spin here until the filtered image
            //    has been delivered to the centroid's in-queue.
            while ( keepWorking )
            {
                // 2a. Get an empty filter buffer, only if we were successful sending the previous one  (ie. fIndex == -1)
                if ( fIndex == -1 )
                {
                    result = mq_read( pongInstance->mqRedEmpty, &fIndex, sizeof(int), 100000 );
                    if ( result!=OK )
                    {
                        if ( result == ERROR )
                            loge("fIndex mq_read failed - %s", strerror(errno) );

                        continue;

                    } /*if*/

                    log("Got empty filter index %d         ", fIndex );
                    //usleep(1000000);

                }/*if*/

                // 2b. Filter video frame to produce filtered frame
                #ifdef PROFILING_ON
                    START_PROFILE();
                #endif

                filterRedFrame((unsigned char *) &pongInstance->videoFrames[vIndex], (unsigned char* ) &pongInstance->redFilters[fIndex], sizeof(FilteredFrame_t));

                #ifdef PROFILING_ON
                    STOP_PROFILE();
                    GET_PROFILE("puckFilter");
                #endif

                // 2c. Send filtered image to the centroid thread
                result = mq_write( pongInstance->mqRedFull, &fIndex, sizeof(int), 100000 );
                if ( result!=OK )
                {
                    if ( result == ERROR )
                        loge("fIndex mq_write failed - %s", strerror(errno) );

                    continue;

                } /*if*/


                log("Put full filter index %d       ", fIndex );
                //usleep(1000000);


                fIndex = -1; //allow another filter receive to happen in the next iteration
                break;

            }/*while*/
        }/*if*/


        // 3. Return used vframe back to the video cap threads
        result = mq_write( pongInstance->mqVideoEmpty, &vIndex, sizeof(int), 100000 );
        if ( result!=OK )
        {
            if ( result == ERROR )
                loge("mq_write failed - %s", strerror(errno) );

            continue;

        }/*if*/


        log("Put back empty video frame index %d       ", vIndex );
        //usleep(1000000);

        vIndex = -1; //allow another receive to happen in the next iteration


    }/*while*/


    log( FG13 "  *** Red-filter exited...");
    return NULL;
}

/****************************************************************************//**
 * Starts the red-filter thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startRedFilter(PongInstance_t* pongInstance)
{
    if( pthread_create( &pongInstance->redFilterThreadId, NULL, redFilterThread, pongInstance ) != 0 )
    {
        loge("Failed to create red-filter thread - %s", strerror(errno) );
        return false;
    }

    return true;
}
