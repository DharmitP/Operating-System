/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
#include <queue.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}



////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
	//threadAddr is null until something acquires it
	lock->threadAddr = NULL;

	//if unlocked isLocked is 0
	lock->isLocked = 0;
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	int spl;
	assert(lock != NULL);

	spl = splhigh();
	assert(thread_hassleepers(lock)==0);
	splx(spl);

	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	int spl;
	assert(lock != NULL);
	assert(in_interrupt == 0);
	
	//disable interrupts
	spl = splhigh();
	while (lock->isLocked == 1){
		thread_sleep(lock);
	}
	lock->isLocked = 1;
	lock->threadAddr = curthread;
	splx(spl);
}

void
lock_release(struct lock *lock)
{
	int spl;
	assert(lock != NULL);
	assert(lock_do_i_hold(lock) == 1);
	
	// disable interrupts
	spl = splhigh();
	
	// release lock and no thread using it
	lock->isLocked = 0;
	lock->threadAddr = NULL;
		
	// wakeup all threads sleeping on lock address 
	thread_wakeup(lock);
	splx(spl);

}

int
lock_do_i_hold(struct lock *lock)
{
	if(lock->threadAddr == curthread)
		return 1;
	else
		return 0;
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	// add stuff here as needed
	cv->s_threads = q_create(1);

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// add stuff here as needed
	q_destroy(cv->s_threads);
	cv->s_threads = NULL;
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	// Write this
	int spl;
	assert(cv != NULL);
	assert(lock != NULL);
	assert(in_interrupt == 0);
	
	//disable interrupts
	spl = splhigh();
	
	//Release the supplied lock, go to sleep, and, after                  
	//waking up again, re-acquire the lock.
	lock_release(lock);
	q_addtail(cv->s_threads, curthread);
	thread_sleep(curthread);
	lock_acquire(lock);

	//enable interrupts
	splx(spl);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	// Write this
	int spl;
	assert(cv != NULL);
	assert(lock != NULL);
	assert(in_interrupt == 0);

	//disable interrupts
	spl = splhigh();
	
	//Wake up one thread that's sleeping on this CV.	
	struct thread *t = q_remhead(cv->s_threads);
	thread_wakeup(t);

	//enable interrupts
	splx(spl);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	int spl;
	assert(cv != NULL);
	assert(lock != NULL);
	assert(in_interrupt == 0);

	//disable interrupts
	spl = splhigh();
	
	//Wake up all threads sleeping on this CV.
	struct thread *t;
	while(!q_empty(cv->s_threads)){
		t = q_remhead(cv->s_threads);
		thread_wakeup(t);
	}
	
	//enable interrupts
	splx(spl);
}
