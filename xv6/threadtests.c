#include "types.h"
#include "user.h"
#include "kthread.h"
#include "stat.h"
#include "fs.h"
#include "uthread.h"
#include "usemaphore.h"


#define STACK_SIZE 500


#define BUFSZ 100
#define ITMNO 1000

int queue[BUFSZ];
int out;
int in;
int finishFlag;
struct csem * empty;
struct csem * full;
struct csem * mutex;


int m1 = 0;
int lock_mutex = 0;

// Thread print
void thread_print()
{
    sleep(10);
    printf(1, "Printing thread \n");
    exit_kthread();
}

// Kthread id print test
void thread_id_print()
{
    sleep(10);
    int result = id_kthread();
    if(result == -1) {
        printf(1, "id_kthread test FAILED! \n");
    }else {
        printf(1, "id_kthread SUCCESS! The id is: %d \n", result);
    }
    exit_kthread();
}

void deallocate_function(int id_mutex)
{
    sleep(10);
    int result = mutex_dealloc_kthread(id_mutex);
    if(result == -1) {
        printf(1, "dealloc_two FAILURE (good)\n");
    }
    else {
        printf(1, "dealloc_one SUCCESS(bad)\n");
    }
    exit_kthread();
}


void unlock_function(int id_mutex)
{
    sleep(10);
    int result = mutex_unlock_kthread(id_mutex);
    if(result == -1) {
        printf(1, "Lock FAILURE, mutex is already unlocked (GOOD). \n");
    }
    else {
        printf(1, "Lock SUCCESS(not GOOD)\n");
    }
    exit_kthread();
}



#define THREAD_FUNCTION(name, id_mutex) \
    void name(){ \
        sleep(10); \
        int result = mutex_dealloc_kthread(id_mutex); \
        if(result == -1) printf(1, "dealloc_two FAILED (good)\n"); \
        else printf(1, "dealloc_one SUCCESS (not good) \n"); \
        exit_kthread(); \
    }

THREAD_FUNCTION(dealloc, m1);

// Kthread create test -- passed
void test_create_kthread()
{
    void *stack = ( (char* ) malloc (STACK_SIZE * sizeof(char))) + STACK_SIZE;

    int tid = create_kthread(thread_print, stack);

    if(tid == -1) {
        printf(1, "create_kthread FAILED! \n");
    }        
    else {
        printf(1, "create_kthread SUCCESS! \n");
    }
    
}

// Kthread join test
void test_join_kthread()
{
    void *stack = ( (char* ) malloc (STACK_SIZE * sizeof(char))) + STACK_SIZE;
     
    int tid = create_kthread(thread_print, stack);
    
    int result = join_kthread(tid);

    if(result == -1) {
        printf(1, "join_kthread FAILED! \n");
    }
    else {
        printf(1, "join_kthread SUCCESS! \n");
    }
    
}

// Kthread id test

void test_id_kthread()
{
    void *stack = ( (char* ) malloc (STACK_SIZE * sizeof(char))) + STACK_SIZE;
    int i;
    for(i = 0; i < 5; i++){
        int tid = create_kthread(thread_id_print, stack);
        join_kthread(tid);
    }
}


void test_mutex_alloc()
{
    printf(1, "Test mutex_alloc_kthread: \n");
    int result = mutex_alloc_kthread();

    if(result == -1) {
        printf(1, "mutex_alloc_kthread FAILURE \n");
    }
    else {
        printf(1, "mutex_alloc_kthread SUCCESS \n");
    }
}


void test_mutex_dealloc()
{
    printf(1, "Test mutex_dealloc_kthread: \n");
    m1 = mutex_alloc_kthread();
    int deall = mutex_dealloc_kthread(m1);

    void *stack = ( (char* ) malloc (STACK_SIZE * sizeof(char))) + STACK_SIZE;

    int tid = create_kthread(dealloc, stack);

    if(deall == 0) {
        printf(1, "dealloc_1 SUCCESS (GOOD) \n");
    }
    else {
        printf(1, "dealloc_1 FAILURE (not GOOD) \n");
    }
     
    join_kthread(tid);
}

void test_mutex_lock_and_unlock()
{
    printf(1, "Test mutex_lock_and_unlock: \n");
    lock_mutex = mutex_alloc_kthread();

    int result = mutex_lock_kthread(lock_mutex);

    if(result == 0) {
        printf(1, "mutex_lock_kthread SUCESS \n");
    }
    else {
        printf(1, "mutex_lock_kthread FAILURE \n");
    }
    

    result = mutex_unlock_kthread(lock_mutex);


    if(result == 0) {
        printf(1, "mutex_unlock_kthread SUCCESS \n");
    }
    else {
        printf(1, "mutex_unlock_kthread FAILURE \n");
    }
    

    void *stack = ( (char* ) malloc (STACK_SIZE * sizeof(char))) + STACK_SIZE;

    int tid = create_kthread(unlock_function, stack);
    join_kthread(tid);
}


// Kthread functions test

void test_kthread_functions()
{
    printf(1, "--- Kernel Level Test ---- \n");
    test_create_kthread(); // works
    test_join_kthread(); // works
    test_id_kthread(); // works
}

void test_kthread_mutex_functions()
{
    printf(1, "--- Kernel Mutex Test ---- \n");
    test_mutex_alloc(); 
    test_mutex_dealloc(); 
    test_mutex_lock_and_unlock();
}




int removeItem(){
	int retNum = queue[out];
	out = (out + 1) % BUFSZ;
	return retNum;
}

void addItem(int item){
	queue[in] = item;
	in = (in + 1) % BUFSZ;
}


void producer(void* arg){
	int i;
	for (i = 1; i <= ITMNO; i++){
		down(empty);
		down(mutex);
		addItem(i);
		up(mutex);
		up(full);
	}
}

void consumer(void* arg){
	while (!finishFlag){
		down(full);
		if (finishFlag) {
			break;
		}
		down(mutex);
		int item = removeItem();
		finishFlag = (item == ITMNO);
		if (finishFlag){
			freeSem(full);	//release blocked thread
		}
		up(mutex);
		up(empty);
		uthread_sleep(item);
		printf(1, "Thread %d slept for %d ticks.\n", uthread_self(), item);
	}
}

int main(int argc, char *argv[])
{
    printf(1, "Test started \n");
    test_kthread_functions();
    test_kthread_mutex_functions();
    exit();

    //initialize threads
	uthread_init();
	//semaphore work for ticks
	empty = allocSem(BUFSZ);
	full = allocSem(0);
	mutex = allocSem(1);
	//create 4 threads
	//3 producer, 1 consumer
	uthread_create(&consumer,0);
	uthread_create(&consumer,0);
	uthread_create(&consumer,0);
	uthread_create(&producer,0);
	uthread_join(1);
	uthread_join(2);
	uthread_join(3);
	uthread_join(4);
	//finish
	freeSem(empty);
	freeSem(mutex);
	uthread_exit();
	return 0;
}