/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "list.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
//Eddy Driving
  old_level = intr_disable ();
  struct thread *currMaxPriorityOnwaitingQ;
  bool waiterRemaining = !list_empty (&sema->waiters);
  if (waiterRemaining) {
    struct list_elem *currMaxPriorityOnwaitingQ_elem = list_max (&sema->waiters, (list_less_func *) isLowerPriority, NULL);
    currMaxPriorityOnwaitingQ = list_entry(currMaxPriorityOnwaitingQ_elem, struct thread, elem);
    list_remove(currMaxPriorityOnwaitingQ_elem);
    thread_unblock(currMaxPriorityOnwaitingQ);
  }
    /*thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));*/
  sema->value++;
  intr_set_level (old_level);

    // If the thread that got added to the ready list has a higher priority than the current thread, current thread yields the CPU
  if (waiterRemaining && thread_current()->priority < currMaxPriorityOnwaitingQ->priority) {
    if (intr_context()) {
      intr_yield_on_return();
    } else {
      thread_yield();
    }
  } 

}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  // Katherine driving
  // Lock held by someone already?
  if(lock->holder != NULL) {

    // Provide mutual exclusion while modifying thread's priority and donation status
    enum intr_level old_level;//disabling interrupts because waiting list of a lock is shared
    old_level = intr_disable ();

    // If my effective priority is higher than holder's effective priority, donate
    if(thread_current()->priority > lock->holder->priority) {
      lock->holder->gotDonation = true;
      lock->holder->priority = thread_current()->priority;
      thread_current()->donee = lock->holder;  // Holder is now my donee

      // Take care of chained priority donation
      // If holder has a donee, keep going down the chain giving my priority to all the donees
      struct thread * curr_thread = lock->holder;

      while(curr_thread->donee != NULL) {
        curr_thread = curr_thread->donee;
        if(curr_thread->priority < thread_current()->priority) {
          curr_thread->priority = thread_current()->priority;
        } else {
          break;  // Oops, I tried to donate to a thread that had a higher priority than me!
        }
      }
    }

    // Daniel Driving

  //Restore interrupts 
  intr_set_level (old_level);
  }

  // Daniel was Driving (code deleted)

  sema_down (&lock->semaphore);

  // When I successfully acquire the lock, set my donee to NULL (I have no need to donate) and add the lock to my list of held locks
  lock->holder = thread_current ();
  lock->holder->donee = NULL;
  list_push_back(&(lock->holder->locks_held), &lock->elem);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  list_remove(&lock->elem);

  // Provide mutual exclusion while modifying thread's priority and gotDonation
  enum intr_level old_level;//disabling interrupts because waiting list of a lock is shared
  old_level = intr_disable ();

  // Katherine driving

  // Was I donated to?
  if(lock->holder->gotDonation) {

    // If I'm holding other locks, get maximum priority from waiters on those locks
    if(!list_empty(&(lock->holder->locks_held))) {

      //Eddy driving
      struct thread * maxPriorityThread_AmongAllLocks = NULL;
      struct list_elem * currThread_elem;
      int currMaxPriority = PRI_MIN;
      struct list_elem* currLock_elem;
      struct lock * currLock;
      struct list currWaitingList;

      // Loop through the locks I'm holding
      for (currLock_elem = list_begin (&(lock->holder->locks_held)); currLock_elem != list_end (&(lock->holder->locks_held)); currLock_elem = list_next (currLock_elem)) {
        currLock = list_entry(currLock_elem, struct lock, elem);

        // Get the maximum priority thread on that lock's waiters list
        currWaitingList = (currLock->semaphore).waiters;

        if (!list_empty(&currWaitingList)) {
          currThread_elem = list_max (&currWaitingList, (list_less_func *) isLowerPriority, NULL);
          struct thread *currThread = list_entry(currThread_elem, struct thread, elem);

          // Compare the priority of the maximum priority thread on the waiters list with the current maximum priority
          if(currThread->priority >= currMaxPriority){
            currMaxPriority = currThread->priority;
            maxPriorityThread_AmongAllLocks = currThread;
          }
        }
      }

      // Daniel driving

      // If one of the threads waiting on my locks can donate to me, take that lock's priority as my own and set me as its donee
      if (maxPriorityThread_AmongAllLocks != NULL && maxPriorityThread_AmongAllLocks->priority > lock->holder->oldPriority) {
        maxPriorityThread_AmongAllLocks->donee = lock->holder;
        lock->holder->priority = currMaxPriority;
      } else {  // If none of the threads waiting on my locks can donate to me, go back to base priority
        lock->holder->gotDonation = false;
        lock->holder->priority = lock->holder->oldPriority;
      }

    } else {  // If not holding other locks, go back to base priority
      lock->holder->gotDonation = false;
      lock->holder->priority = lock->holder->oldPriority;
    }
  }

  // Done modifying thread's priority and gotDonation
  //Restore interrupts 
  intr_set_level (old_level);

  // Daniel Driving

  // Release lock
  lock->holder = NULL;
  
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

// Removed semaphore_elem struct, defined in synch.h instead

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  waiter.currUsingThread = thread_current();
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) {

    // Katherine driving
    // Get waiter for the thread with the maximum priority
    struct list_elem *currMaxPriorityWaiter_elem = list_max (&cond->waiters, (list_less_func *) isLowerPriority_S, NULL);
    struct semaphore_elem *currMaxPriorityWaiter = list_entry(currMaxPriorityWaiter_elem, struct semaphore_elem, elem);

    // Remove from list of waiters
    list_remove (currMaxPriorityWaiter_elem);

    // Release the semaphore
    sema_up (&(currMaxPriorityWaiter->semaphore));
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
