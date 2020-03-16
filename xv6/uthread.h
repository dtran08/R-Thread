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

//similart struct tpes for these mutex types
typedef struct mutex mutex_t, *mutex_p;

//array of all threads, under type thread_t
static thread_t all_thread[MAX_THREAD];

//next, current thread references
thread_p current_thread;
thread_p next_thread;

extern void thread_switch(thread_p, thread_p);

boolean thread_create(char* name, void *func) {
  thread_p t;
  boolean free = false;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if(t->state == FREE) {
        free=true;
        break;
      }
  }
  if(free==false) {
    printf(2,"Thread \'%s\'creation unsuccessful. MAX_THREAD limit reached \n",name);
    return false;    // no space for threads available, max_threads limit reached
  }

  else {
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

  t = current_thread;
  while(t < all_thread + MAX_THREAD) {       // first search if a runnable thread is present after current_thread
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      found = true;
      break;
    }
    t++;
  }

  if(found==false) {                         // if not found, search from beginning to position of current_thread
    t = all_thread;
    while(t != current_thread) {
      if (t->state == RUNNABLE) {
        next_thread = t;
        found = true;
        break;
      }
      t++;
    }
  }

  if(current_thread->state==RUNNABLE) {       // now if found is false, that means t = current_thread is only runnable thread
    next_thread = t;
  }

  if (next_thread == 0) {                     // as assigned previously to 0 during switches
    printf(2, "thread_schedule: no more runnable threads;\n");
    exit();
  }

  if (current_thread != next_thread) {        // switch threads in this case
    next_thread->state = RUNNING;
    thread_switch(current_thread, next_thread);
  } 
  else{
    next_thread = 0;
  }
}

void thread_yield(void) {
  current_thread->state = RUNNABLE;
  thread_schedule();
}

//lock/mutex things
void lock_busy_wait_acquire(mutex_p lock) {//yield the thread if the lock is acquired
  while(lock->acq == true) {
    thread_yield();
  }
  lock->acq = true;
}

void lock_busy_wait_release(mutex_p lock) {
  lock->acq = false;
}


void lock_acquire(mutex_p lock) {//acquire lock
  if(lock->acq == false) {
    lock->acq = true;
  }
  else {
    current_thread->state = WAITING;
    lock->queue[(lock->back + 1) % MAX_THREAD] = current_thread;
    lock->back = (lock->back + 1) % MAX_THREAD;     
    lock->filled++;
    thread_schedule();
  }
}

void lock_release(mutex_p lock) {//release lock
  lock->acq = false;
  if(lock->filled != 0) {
    lock->queue[lock->front]->state = RUNNABLE;
    lock->front = (lock->front+1)%MAX_THREAD;
    lock->filled--;
  }
}

void thread_init(void) {
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void init_lock(mutex_p lock) {
  lock->acq=false;
  lock->front=0;
  lock->back=0;
  lock->filled=0;
}