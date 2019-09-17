/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
 * this file.
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
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

//number of times a cat and mouse eats
#define NCATEATS 4
#define NMOUSEEATS 4

// Global variable definitions
static struct lock *countLock;
static struct lock *lockDish1;
static struct lock *lockDish2;
static struct lock *lockDishSelect;
volatile int numDishesAvailable = 2;
static struct cv *isDishFree;
volatile int numCatsEating = 0;
volatile int numMiceEating = 0; 
volatile int numThreadsRunning = 8;

/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
lock_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

       	// (void) unusedpointer;
       	// (void) catnumber;

		// ADDED CODE START
		int i;
		for(i = 0; i < NCATEATS; i++){
			lock_acquire(lockDishSelect);
			while(numDishesAvailable <= 0 || numMiceEating > 0){
				cv_wait(isDishFree, lockDishSelect);
			}
			if(lockDish1->isLocked == 0){
				lock_acquire(lockDish1);
				numDishesAvailable--;
				numCatsEating++;
				lock_release(lockDishSelect);
				cv_broadcast(isDishFree, lockDishSelect);
				lock_eat("cat", catnumber, 1, i);
				numDishesAvailable++;
				numCatsEating--;
				lock_release(lockDish1);			
			}
			else if(lockDish2->isLocked == 0){
				lock_acquire(lockDish2);  
				numDishesAvailable--;
				numCatsEating++;    
				lock_release(lockDishSelect);
				cv_broadcast(isDishFree, lockDishSelect);
				lock_eat("cat", catnumber, 2, i);
				numDishesAvailable++;
				numCatsEating--;    
				lock_release(lockDish2);			
			}
			cv_broadcast(isDishFree, lockDishSelect);
		}
		lock_acquire(countLock);
		numThreadsRunning--;
		lock_release(countLock);
}
	

/*
 * mouselock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        // (void) unusedpointer;
        // (void) mousenumber;
		
		// ADDED CODE START
		int i;
		for(i = 0; i < NMOUSEEATS; i++){
			lock_acquire(lockDishSelect);
			while(numDishesAvailable <= 0 || numCatsEating > 0){
				cv_wait(isDishFree, lockDishSelect);
			}
			if(lockDish1->isLocked == 0){
				lock_acquire(lockDish1);
				numDishesAvailable--;
				numMiceEating++;    
				lock_release(lockDishSelect);
				cv_broadcast(isDishFree, lockDishSelect);
				lock_eat("mouse", mousenumber, 1, i);
				numDishesAvailable++;
				numMiceEating--;    
				lock_release(lockDish1);			
			}
			else if(lockDish2->isLocked == 0){
				lock_acquire(lockDish2);  
				numDishesAvailable--;    
				numMiceEating++;    
				lock_release(lockDishSelect);
				cv_broadcast(isDishFree, lockDishSelect);
				lock_eat("mouse", mousenumber, 2, i);
				numDishesAvailable++;
				numMiceEating--;    
				lock_release(lockDish2);			
			}
			cv_broadcast(isDishFree, lockDishSelect);
		}
		lock_acquire(countLock);
		numThreadsRunning--;
		lock_release(countLock);
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        // (void) nargs;
        // (void) args;
   		
		// create locks
		lockDish1 = lock_create("lockDish1");
		lockDish2 = lock_create("lockDish2");
		lockDishSelect = lock_create("lockDishSelect");
		countLock = lock_create("countLock");

		// create cv
		isDishFree = cv_create("isDishFree");

        /*
         * Start NCATS catlock() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

		while(numThreadsRunning > 0){
		}
		
        return 0;
}

/*
 * End of catlock.c
 */
