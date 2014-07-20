/****************************************************************************//**\file
 * Main program to launch pong.
 *
 * \mainpage
 * <h2>Course:</h2>
 * Real-time Embedded Systems ECEN5623<br/>
 * Fall 2013
 *
 * <h2>Group Members:</h2>
 * Bhaumik Bhatt<br/>
 * Edwin R. Lopez<br/>
 * Mayank Bhardwaj<br/>
 * Tejas Joshi<br/>
 * Vinit Vyas<br/>
 ****************************************************************************/
#include "pong.h"


/****************************************************************************//**
 * Displays program's usage information and exits.
 * @param progName Usually argv[0], the name of the executable.
 ****************************************************************************/
static void usage( const char *progName )
{
    fprintf( stderr, "Usage: %s [-b] [-1|2|3] [-f logfilter]\n\n", progName );
    fprintf( stderr, " -b        Start pong using gpio hardware in the B-side of the table.\n"
                     "           If this option is not present, pong defaults to the A-Side of the table.\n\n" );
    fprintf( stderr, " -f        Specifies a log filter. For example the expression \"y=|time\".\n"
                     "           makes the log show entries containing the patterns 'y=' or 'time'\n\n" );
    fprintf( stderr, " -1|-2|-3  Set the logging level: 1=warn, 2=err 3=none. The default is all. \n\n" );
    fprintf( stderr, " -p        Purge MQ and exits. Useful after a crash where the MQ are left unlinked.\n\n" );

    fprintf( stderr, "\n" );
    exit( -1 );
}


/****************************************************************************//**
 * Pong main program. Parses the command line options, configure the
 * global variable environment and starts program threads.
 ****************************************************************************/
int main( int argc, char **argv )
{
    puts( FG15 "Pong version 1.0" );
    puts( "Group7: Real-Time Embedded Systems, ECEN5623 Fall 2013\n" NOC );

    // 1. Parse command-line options
    int   opt;                          // Used to iterate through letter options on the command line
    //bool  usingSideA = true;            // Flag used to determine which side (A or B) we are running pong (adjusted by command line options below)
    bool  mqPurgueAndQuit = false;      // Flag set true is we are just cleaning the MQs and exitingq (adjusted by command line options below)

    while ( (opt = getopt( argc, argv, "b123f:p" )) != -1 ) // Tip, a ':' following an option means the option has an argument 'optarg'
    {                                                       // The global variable 'optarg' contains the string value passed as option argument, also you can use atoi(optarg) to convert that value to int
        switch ( opt )
        {
            // Purge MQ and exits
            case 'p':
                mqPurgueAndQuit = true;
                break;

            /*// Specifies side B of the table
            case 'b':
                usingSideA = false;
                break;
             */

            // Enable custom log filtering
            case 'f':
                logSetFilter( optarg );
                break;

            // Set logging level:
            case '1':
            case '2':
            case '3':
                logSetLevel( opt-0x30 ); //0=info, 1=warn, 2=err 3=none
                break;

            // If we find an option we don't support, display usage and quit
            default:
                usage( argv[0] );

        }/*switch*/

    }/*while*/


    //Configure the global configuration structure 'pongInstance' to be used in either side A or B

        configurePongInstances();

        // 3. Purging the mq? do it and quit...
        if ( mqPurgueAndQuit )
        {
            mqPurgueAndQuit = false;
            //clean up the queues
            stopPong(&pongInstances[0]);
            //puts("Done closing 0");
            stopPong(&pongInstances[1]);
            puts("Purge done.");
            exit(0);
        }

        //START_PROFILE();
        //usleep(10000);
        //STOP_PROFILE();
        //GET_PROFILE("test");

        //Setup application environment, variable initial state
        if ( !(setupPong(&pongInstances[0]) && setupPong(&pongInstances[1])) )
        {
            loge("Failed to setup program environment.");
            exit(-1);
        }

        //Start all threads an let them do the work while we just hang in here until the user enters any input and hit enter
        if ( !(startPong(&pongInstances[0]) && startPong(&pongInstances[1])) )
        {
            loge("Failed to start program threads.");
            //stop any partially started threads
            stopPong(&pongInstances[0]);
            stopPong(&pongInstances[1]);
            exit(-1);
        }

        log("Pong is running (Type anything and press enter to exit)\n");
        getchar(); //dummy read

        //Clean up and exit
        log("Terminating program...");
        stopPong(&pongInstances[0]);
        stopPong(&pongInstances[1]);

    log("bye!");
    return 0;
}
