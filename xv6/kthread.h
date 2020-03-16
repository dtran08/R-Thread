#define MAX_STACK_SIZE  4000
#define MAX_MUTEXES 64


// --  Basic Kthread functions -- // 

//  Create new thread in calling process = RUNNABLE
//  Caller must allocate user stack for new thread, 
//  size defined with MAX STACK SIZE
//  start_function = pointer to entry function that thread will execute

//  Returns identifier of newly created thread
//  Else non-positive value returned
int create_kthread(void (*start_function)(), void* stack);
//  Returns the caller thread's id.
//  Else returns a negative error code.
    //  Thread ID and Process ID are generally different 
int id_kthread();
//  Terminates the  execution of calling thread 
//  If called by thread (even main) while other threads exist w/in same process
//  shouldn't terminate whole process
//  If last running thread, process should terminate.
//  Each thread must call this function to terminate
void exit_kthread();
//  Suspends execution of calling thread until target thread (id_thread) terminated
//  If thread already exited, execution shouldn't be suspended 
//  Returns zero
//  Else, returns -1 on error
int join_kthread(int id_thread);


// Mutex kthread functions

int mutex_alloc_kthread();
int mutex_dealloc_kthread(int id_mutex);
int mutex_lock_kthread(int id_mutex);
int mutex_unlock_kthread(int id_mutex);