/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

/*
 *
 * Constants
 *
 */

/*
 * Number of cars created.
 */

#define NCARS 20

// Global variables
static struct semaphore *intersectionSem;
static struct semaphore *NWSem;
static struct semaphore *NESem;
static struct semaphore *SWSem;
static struct semaphore *SESem;
static struct semaphore *countThreads;
volatile int numThreadsRunningTraffic = 20;

/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        // (void) cardirection;
        // (void) carnumber;
		
		//ADDED CODE STARTS
		
		// approaches from N
		if(cardirection == 0){
			message(0, carnumber, cardirection, 2);			
			P(NWSem);
			message(1, carnumber, cardirection, 2);
			P(SWSem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 2);
			V(NWSem);
			message(4, carnumber, cardirection, 2);
			V(SWSem);		
		}

		// approaches from E
		else if(cardirection == 1){
			message(0, carnumber, cardirection, 3);			
			P(NESem);
			message(1, carnumber, cardirection, 3);
			P(NWSem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 3);
			V(NESem);
			message(4, carnumber, cardirection, 3);
			V(NWSem);		
		}

		// approaches from S
		else if(cardirection == 2){
			message(0, carnumber, cardirection, 0);			
			P(SESem);
			message(1, carnumber, cardirection, 0);
			P(NESem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 0);
			V(SESem);		
			message(4, carnumber, cardirection, 0);
			V(NESem);
		}

		// approaches from W
		else if(cardirection == 3){
			message(0, carnumber, cardirection, 1);			
			P(SWSem);
			message(1, carnumber, cardirection, 1);
			P(SESem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 1);
			V(SWSem);
			message(4, carnumber, cardirection, 1);
			V(SESem);
		}
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        // (void) cardirection;
        // (void) carnumber;
		
		// approaches from N
		if(cardirection == 0){
			message(0, carnumber, cardirection, 1);			
			P(NWSem);
			message(1, carnumber, cardirection, 1);
			P(SWSem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 1);
			V(NWSem);
			P(SESem);	
			message(3, carnumber, cardirection, 1);
			V(SWSem);
			message(4, carnumber, cardirection, 1);
			V(SESem);
		}

		// approaches from E
		else if(cardirection == 1){
			message(0, carnumber, cardirection, 2);			
			P(NESem);
			message(1, carnumber, cardirection, 2);
			P(NWSem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 2);
			V(NESem);
			P(SWSem);				
			message(3, carnumber, cardirection, 2);
			V(NWSem);		
			message(4, carnumber, cardirection, 2);
			V(SWSem);
		}

		// approaches from S
		else if(cardirection == 2){
			message(0, carnumber, cardirection, 3);			
			P(SESem);
			message(1, carnumber, cardirection, 3);
			P(NESem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 3);
			V(SESem);
			P(NWSem);
			message(3, carnumber, cardirection, 3);				
			V(NESem);
			message(4, carnumber, cardirection, 3);
			V(NWSem);
		}

		// approaches from W
		else if(cardirection == 3){
			message(0, carnumber, cardirection, 0);			
			P(SWSem);
			message(1, carnumber, cardirection, 0);
			P(SESem);  // make sure to acquire next region before leaving current one
			message(2, carnumber, cardirection, 0);
			V(SWSem);
			P(NESem);
			message(3, carnumber, cardirection, 0);				
			V(SESem);
			message(4, carnumber, cardirection, 0);
			V(NESem);
		}
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        // (void) cardirection;
        // (void) carnumber;
		
		// approaches from N
		if(cardirection == 0){
			message(0, carnumber, cardirection, 3);			
			P(NWSem);
			message(1, carnumber, cardirection, 3);
			message(4, carnumber, cardirection, 3);
			V(NWSem);
		}

		// approaches from E
		else if(cardirection == 1){
			message(0, carnumber, cardirection, 0);			
			P(NESem);
			message(1, carnumber, cardirection, 0);
			message(4, carnumber, cardirection, 0);
			V(NESem);
		}

		// approaches from S
		else if(cardirection == 2){
			message(0, carnumber, cardirection, 1);			
			P(SESem);
			message(1, carnumber, cardirection, 1);
			message(4, carnumber, cardirection, 1);
			V(SESem);
		}

		// approaches from W
		else if(cardirection == 3){
			message(0, carnumber, cardirection, 2);			
			P(SWSem);
			message(1, carnumber, cardirection, 2);
			message(4, carnumber, cardirection, 2);
			V(SWSem);
		}	
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;
		int turnDirection;

        /*
         * Avoid unused variable and function warnings.
         */

        // (void) unusedpointer;
        // (void) carnumber;
	// (void) gostraight;
	// (void) turnleft;
	// (void) turnright;

        /*
         * cardirection is set randomly.
         */

        cardirection = random() % 4;

		// turnDirection set randomly
		turnDirection = random() % 3; // 3 possible turnDirections (0 = straight, 1 = left, 2 = right)

		P(intersectionSem);
		if(turnDirection == 0)
			gostraight(cardirection, carnumber);
		else if(turnDirection == 1)
			turnleft(cardirection, carnumber);
		else if(turnDirection == 2)
			turnright(cardirection, carnumber);
		V(intersectionSem);

		// leave intersection; decrease thread count
		P(countThreads);
		numThreadsRunningTraffic--;
		V(countThreads);
}	


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

		// create semaphores
		intersectionSem = sem_create("intersectionSem", 3); // only allow 3 cars in the intersection at all times
		NWSem = sem_create("NWSem", 1);  // only 1 car per region inside the intersection
		NESem = sem_create("NESem", 1);
		SWSem = sem_create("SWSem", 1);
		SESem = sem_create("SESem", 1);
		countThreads = sem_create("countThreads", 1);

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }
		
		while(numThreadsRunningTraffic > 0){
		}

        return 0;
}
