#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "uthread.h"
#include "usemaphore.h"

#define BUFSZ 100
#define ITMNO 1000

int queue[BUFSZ];
int out;
int in;
int finishFlag;
struct counting_semaphore * empty;
struct counting_semaphore * full;
struct counting_semaphore * mutex;

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

int main(int argc, char *argv[]){
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
