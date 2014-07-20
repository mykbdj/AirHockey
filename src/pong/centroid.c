/****************************************************************************//**\file
 * Contains the centroid thread business logic implementation.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 * References: http://ecee.colorado.edu/~ecen5623/ecen/ex/ecen5623/RTECS-CDROM/Image-Processing/image-enhancement/centroid.c
 ****************************************************************************/
#include "pong.h"



/****************************************************************************//**
 * Centroid thread main entry point.
 * @param arg Optional thread parameter, this is unused.
 * @return Always null
 ****************************************************************************/
static void *centroidThread( void *arg )
{

    PongInstance_t* pongInstance = (PongInstance_t*) arg;

    Coords_t coords;
    int      fIndex = -1; // Input Filter buffer
    int      result;

    while ( keepWorking )
    {
        // 1. Get a full filter buffer, only if we were successful releasing the previous one  (ie. fIndex == -1)
        if ( fIndex == -1 )
        {
            result = mq_read( pongInstance->mqRedFull, &fIndex, sizeof(int), 100000 );
            if ( result!=OK )
            {
                if ( result == ERROR )
                    loge("fIndex mq_read failed - %s", strerror(errno) );

                continue;

            } /*if*/

            log("Got full filter index %d         ", fIndex );
            //usleep(1000000);

        }/*if*/


        // 2. Process filter image and find puck centroid
        // copying  x, y to coords
        #ifdef PROFILING_ON
            START_PROFILE();
        #endif

        if(!puckCentroid((unsigned char* ) &pongInstance->redFilters[fIndex], VFRAME_WIDTH, VFRAME_HEIGHT, &coords, pongInstance->id) )
            loge("Failed to get Puck Centroid");

        #ifdef PROFILING_ON
            STOP_PROFILE();
            GET_PROFILE("getPuckPos");
        #endif

        // 3. Send X,Y coords to trajectory thread

        result = mq_write( pongInstance->mqXYCoords, &coords, sizeof(coords), 100000 );

        if ( result==ERROR )
            loge("mqXYCoords mq_write failed - %s", strerror(errno) );
        else if ( result==TIMEOUT )
            loge("mqXYCoords mq_write timeout - %s", strerror(errno) );
        else
            log("mqXYCoords sent x,y=(%d, %d)         ", coords.x, coords.y );

        // 4. Return used filtered image back to red filter thread
        result = mq_write( pongInstance->mqRedEmpty, &fIndex, sizeof(int), 100000 );
        if ( result!=OK )
        {
            if ( result == ERROR )
                loge("fIndex mq_write failed - %s", strerror(errno) );

            continue;

        } /*if*/


        log("Put back empty filter index %d       ", fIndex );
        //usleep(1000000);


        fIndex = -1; //allow another filter receive to happen in the next iteration

    }/*while*/


    log( FG13 "  *** Centroid exited...");
    return NULL;
}

/****************************************************************************//**
 * Starts the centroid thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startCentroid(PongInstance_t* pongInstance)
{
    if( pthread_create( &pongInstance->centroidThreadId, NULL, centroidThread, pongInstance ) != 0 )
    {
        loge("Failed to create centroid thread - %s", strerror(errno) );
        return false;
    }

    return true;
}
