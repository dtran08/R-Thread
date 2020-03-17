#define true 1
#define false 0
#define boolean _Bool

//define states
#define FREE        0
#define RUNNING     1
#define RUNNABLE    2
#define WAITING     3

//max num threads, stack bit size
#define STACK_SIZE  8192
#define MAX_THREAD  5

struct thread {
  int sp;//sp stack pointer
  char stack[STACK_SIZE];//total thread stack for sched
  int state;//refer to states above
  char *name;//thread name
};

//similar struct types for these thread types
typedef struct thread thread_t, *thread_p;

//mutex
struct mutex {
  boolean acq;
  thread_p queue[MAX_THREAD];   // to store waiting threads for this lock
  int front;
  int back;
  int filled;
};

//similar struct tpes for these mutex types
typedef struct mutex mutex_t, *mutex_p;

//array of all threads, under type thread_t
static thread_t all_thread[MAX_THREAD];

//next, current thread references
thread_p current_thread;
thread_p next_thread;

extern void thread_switch(thread_p, thread_p);

//cureate a new thread with a running extra function (func optional)
boolean thread_create(char* name, void *func) {
  thread_p t;
  boolean free = false;

  //t = a thread representing the array all_thread
  //t < 10
  //if the state of the thread is currently free, the bool free is true
  //checks for any instances of a free thread in memory in the array
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if(t->state == FREE) {
        free=true;
        break;
      }
  }
  if(free==false) {//if there are no free threads, we terminate and return error
    printf(2,"Thread \'%s\'creation unsuccessful. MAX_THREAD limit reached \n",name);
    return false;    // no space for threads available, max_threads limit reached
  }

  else {//stack things
    t->sp = (int) (t->stack + STACK_SIZE);   // set sp to the top of the stack
    t->sp -= 4;                              // space for return address
    * (int *) (t->sp) = (int)func;           // push return address on stack
    t->sp -= 32;                             // space for registers that thread_switch will push
    t->state = RUNNABLE;
    t->name = name;
    printf(2,"Thread \'%s\' successfully created \n",t->name);
    return true;
  }
}


void thread_schedule(void) {//schedule based on round robin
  thread_p t;
  boolean found = false;

  t = current_thread;//our thread var t is the current thread
  //parse through the all_thread starting from current location of t
  while(t < all_thread + MAX_THREAD) {
    //if the state of the NEXT t is runnable and it's NOT the current thread
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;//assign the next thread to t
      found = true; //we've found the next thread
      break;//we're done
    }
    t++;//loop incremental var
  }

  //if we went through the all_thread and didn't find a RUNNABLE thread
  if(found==false) { 
    t = all_thread;//start at beginning of all_thread
    while(t != current_thread) {//do the same thing as before but from the start to finish
      if (t->state == RUNNABLE) {
        next_thread = t;
        found = true;
        break;
      }
      t++;
    }
  }

  //given found stays false, that means the current thread is t and is the only RUNNABLE thread
  if(current_thread->state==RUNNABLE) { 
    next_thread = t;
  }

  //case if there are no more runnable threads
  if (next_thread == 0) {
    printf(2, "thread_schedule: no more runnable threads;\n");
    exit();
  }

  //if the current thread is not the next thread
  //we move forward and assign the next state to RUNNING
  //switch threads
  //if the current thread IS the next thread, we're done
  if (current_thread != next_thread) {        // switch threads in this case
    next_thread->state = RUNNING;
    thread_switch(current_thread, next_thread);
  } 
  else{
    next_thread = 0;
  }
}

//sets the current thread's state to RUNNABLE and run through the scheduler
//usually a wait function
void thread_yield(void) {
  current_thread->state = RUNNABLE;
  thread_schedule();
}


//lock/mutex things

//a busy-waiting model; the thread that's trying to acquire the lock WILL fail, and yield until it's ready
//while the lock is acquired (true), keep yielding
//once the lock is not acquired (false), assign it to true
//this function waits for the lock to be released before relocking it for the next thread
void lock_busy_wait_acquire(mutex_p lock) {//yield the thread if the lock is acquired
  while(lock->acq == true) {
    thread_yield();
  }
  lock->acq = true;
}

//releases the lock (acquired if false), putting it up for locking
void lock_busy_wait_release(mutex_p lock) {
  lock->acq = false;
}

//acquire/lock the lock (true) if the lock isn't already locked (acquired)
void lock_acquire(mutex_p lock) {
  if(lock->acq == false) {
    lock->acq = true;
  }
  else {//if the lock is already acquired, we need to wait on whatever's running it
    //the current thread's state is WAITING to be released
    current_thread->state = WAITING;
    //the queue (originally of size MAX_THREAD)
    //queue AT back+1 % MAX_THREAD, aka back+1, is assigned to current_thread
    lock->queue[(lock->back + 1) % MAX_THREAD] = current_thread;
    //reassign lock->back to its value in the queue for current_thread
    lock->back = (lock->back + 1) % MAX_THREAD;    
    //increment filled to say that there's a lock that's filled 
    lock->filled++;
    //schedule the next thread
    thread_schedule();
  }
}

//release our lock, undoing lock_acquire; ONLY works if lock_acquire worked
void lock_release(mutex_p lock) {
  //release our lock!
  lock->acq = false;
  //if our lock IS FILLED, aka not 0
  if(lock->filled != 0) {
    //the lock at the previous state is now RUNNABLE
    lock->queue[lock->front]->state = RUNNABLE;
    //reassign our lock's front to its value % MAX_THREAD, or just the front var
    lock->front = (lock->front+1)%MAX_THREAD;
    //decrement the filled var, usually it's 1 so it shouldn't be a problem
    lock->filled--;
  }
}

//initialize our thread
void thread_init(void) {
  //we start our current_thread at the original point of all_thread
  current_thread = &all_thread[0];
  //it's RUNNING since it's the first one!
  current_thread->state = RUNNING;
}

//initialize our mutex
void init_lock(mutex_p lock) {
  lock->acq=false;
  //for our release; enables the thread state to be RUNNABLE and increments itself
  lock->front=0;
  //for our acquisition; after our current_thread is WAITING, the first pthread element is the current_thread
  //allow it to increment
  lock->back=-1;
  //numerical acquisition of lock
  lock->filled=0;
}