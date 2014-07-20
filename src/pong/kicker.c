/****************************************************************************//**\file
 * Contains the kicker/laser thread business logic implementation.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"


/****************************************************************************//**
 * Kicker thread main entry point. This thread is responsible for monitoring
 * an interruption on the laser path and firering the solenoid kicker 
 * accordingly.
 *  
 * @param arg The pong instance.
 * @return Always null
 ****************************************************************************/
static void *kickerThread( void *arg )
{
    PongInstance_t* pongInstance = (PongInstance_t*) arg;
    int delayCounter = 0;
    int pinValue;


    log( FG13 "Kicker %d started...", pongInstance->id );

    while ( keepWorking )
    {
  
//        gpioWritePin( 44 , HIGH );
//        usleep(1000000);
//        gpioWritePin( 44 , LOW );
//        usleep(1000000);

        usleep(4);

        pinValue = gpioReadPin( pongInstance->laserSensorPin );

        if ( pinValue == 1 )
            delayCounter++;
        else
            delayCounter = 0;

        if ( delayCounter > 6 )
        {
            gpioWritePin( pongInstance->kickerPin, HIGH );
            usleep(100000);
            delayCounter = 0;

            gpioWritePin( pongInstance->kickerPin, LOW );

            // get suck here until laser beam is restored
            while ( keepWorking && gpioReadPin( pongInstance->laserSensorPin ) )
            {
                usleep(100000);
            }

        }

    }/*while*/
    
    gpioWritePin( pongInstance->kickerPin, LOW );

    log( FG13 "  *** Kicker exited...");
    return NULL;

}

    
/****************************************************************************//**
 * Starts the kicker thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startKicker(PongInstance_t* pongInstance)
{
    if( pthread_create( &pongInstance->kickerThreadId, NULL, kickerThread, pongInstance ) != 0 )
    {
        loge("Failed to create kicker thread - %s", strerror(errno) );
        return false;
    }

    return true;
}
