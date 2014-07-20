/****************************************************************************//**\file
 * Contains the actuator thread business logic implementation.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"



/****************************************************************************//**
 * SIGUSR1 Signal handler for receiving information from the trajectory and
 * paddle tracker threads. Also we receive here notifications from the ISR
 * when a limit switch or the laser sensor has been tripped.
 ****************************************************************************/
/*
void signalHandler(int sigNo, siginfo_t *info, void *ucontext )
{
    int pongId = sigNo==SIGUSR1 ? 0 : 1;

    // The upper byte in the sival_int indicates where the value comes from
    // is the MSB is 1, the sender is the paddle tracker, if it is 2 the senter is
    // the trajectory thread. Other MSB values indicate a signal send by the ISR.
    switch ( info->si_value.sival_int >> 24 )
    {
        case 1: pongInstances[pongId].paddlePos = info->si_value.sival_int & 0xFFFF; // use the lower 16-bit as the paddle position
                logw("Actuator received paddlePos=%d", pongInstances[pongId].paddlePos );
                break;

        case 2: pongInstances[pongId].predictedPos = info->si_value.sival_int & 0xFFFF; // use the lower 16-bit as the trajectory prediction
                logw("Actuator received predictedPos=%d", pongInstances[pongId].predictedPos );
                break;

        default:
                // ISR?
                break;
    }

    pongInstances[pongId].newData = 1;
}
*/


/****************************************************************************//**
 * Actuator thread main entry point.
 * @param arg Optional thread parameter, this is unused.
 * @return Always null
 ****************************************************************************/
static void *actuatorThread( void *arg )
{
    volatile PongInstance_t* pongInstance = (PongInstance_t*) arg;
    unsigned int  pos, pred;
    int tem=0, tem1;
    int span=0, accel=900, toggle=0, i=0;

    int moveOneGrid(int STEP);
    //static arduino_toggle=0;

    gpioWritePin( pongInstance->dirPin, HIGH );


    /************************************************************************************************************************/


while ( keepWorking )
{

    //if( gpioReadPin( pongInstance->limitSwitchPin ) ==0 )
    {
        pongInstance->newData = 0;

        pred=pongInstance->predictedPos;
        pos=pongInstance->paddlePos;
        if(pred == 25) pred=9;
        else if(pred>16) pred=17;
        else if(pred<2) pred=1;



        tem = pred-pos;
        tem1=abs(tem);

       // tem1--;
        loge("%s Value of tem %d  tem1=%d    pred=%d, pos=%d        ",pongInstance->id ? "SideB": "SideA" , tem, tem1, pred, pos);

        if(pongInstance->id==0)
               gpioWritePin( pongInstance->dirPin, (pred> pos)? LOW:HIGH );
           else
               gpioWritePin( pongInstance->dirPin, (pred> pos)? HIGH:LOW );

       if(tem1 != 0)
       {
           logwxy(1,4,"%s Value of tem %d  pred=%d   -pos=%d  ",pongInstance->id ? "SideB": "SideA" , tem, pred, pos);
              //arduino_toggle =1;

//            if ( span++ > accel )
//            {

//           for(i=0;i<10;i++)
//           {
                //toggle = !toggle;
                //usleep( 10 );
                //log("Moving");
                gpioWritePin( pongInstance->stepPin, HIGH);
                //toggle = !toggle;

                //usleep( 1 );
//                gpioWritePin( pongInstance->stepPin, toggle ? HIGH : LOW );
//                toggle = !toggle;
//                usleep(1);
//                gpioWritePin( pongInstance->stepPin, toggle ? HIGH : LOW );
//                toggle = !toggle;
//                usleep( 250 );
//                gpioWritePin( pongInstance->stepPin, toggle ? HIGH : LOW );
//                toggle = !toggle;
//                usleep(250);
//                gpioWritePin( pongInstance->stepPin, toggle ? HIGH : LOW );
//                toggle = !toggle;
//                usleep( 250 );
//                gpioWritePin( pongInstance->stepPin, toggle ? HIGH : LOW );
//                toggle = !toggle;
//                usleep(250);

//           }
//                span = 0;
//                if ( accel > 400 )
//                    accel -= 0.2;
        }
       else
       {
           //arduino_toggle = 0;
           gpioWritePin( pongInstance->stepPin, LOW);
       }

//        else
//        {
//            gpioWritePin( pongInstance->dirPin, HIGH );
//            log("Position reached");
//        }

    }


}/*while*/

    log( FG13 "  *** Actuator exited...");
    return NULL;
}

/****************************************************************************//**
 * Starts the actuator thread.
 * @return true if the thread started successfully, false otherwise.
 ****************************************************************************/
bool startActuator(PongInstance_t* pongInstance)
{
/*
    struct sigaction    action;

    // 1. attach signal to the handler function
    memset (&action, 0, sizeof(action) );  // Zero all fields in structure (clean up)
    action.sa_flags = SA_SIGINFO;          // Indicate use of callback attached to field 'sa_sigaction' should be used, not callback in 'sa_handler'
    action.sa_sigaction = signalHandler;   // Set callback function
    sigemptyset(&action.sa_mask);          // Block all signals while we are in the handler. By default our signal
                                           //   is mask blocked unless the SA_NODEFER flag is set in sa_flags
    if ( sigaction( pongInstance->actuatorSignal, &action, NULL) != 0 )
    {
        loge("Actuator cannot set  signal handler - %s\n", strerror(errno) );
        return false;
    }
*/


    // 2. Start actuator thread
    if( pthread_create( &pongInstance->actuatorThreadId, NULL, actuatorThread, pongInstance ) != 0 )
    {
        loge("Failed to create actuator thread - %s", strerror(errno) );
        return false;
    }

    return true;
}
