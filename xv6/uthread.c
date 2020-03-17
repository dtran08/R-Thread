#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

//define locks
mutex_p lock1;
mutex_p lock2;


void firstThread(void) {
  lock_acquire(lock1); //begin crit selec, LOCK!
  printf(1, "Thread \'%s\' has started running\n",current_thread->name);

  int i;
  for (i = 0; i < 10; i++) {
    printf(1, "Thread \'%s\' at 0x%x\n", current_thread->name, (int) current_thread);
    thread_yield(); //yield at crit selec, allow next thread to run
  }
  printf(1, "Thread \'%s\' has finished; exititing\n",current_thread->name);
  
  current_thread->state = FREE;//reassign state
  lock_release(lock1);//end crit selec, UNLOCK!
  thread_schedule();
}



void secondThread(void) {
  lock_acquire(lock2);//begin crit selec, LOCK!
  printf(1, "Thread \'%s\' has started running\n",current_thread->name);
  
  int i;
  for (i = 0; i < 10; i++) {
    printf(1, "Thread \'%s\' at 0x%x\n", current_thread->name, (int) current_thread);
    thread_yield();//yield at crit selec, allow next thread to run
  }
  
  printf(1, "Thread \'%s\' has finished; exiting\n",current_thread->name);
  
  current_thread->state = FREE;//reassign state
  lock_release(lock2);//end crit selec, UNLOCK!
  thread_schedule();
}


int main(int argc, char *argv[]) {
  thread_init();
  mutex_t mutex1, mutex2;

  lock1 = &mutex1;//assign mutexes
  lock2 = &mutex2;

  init_lock(lock1);//init lock since queue
  init_lock(lock2);

  //because firstThread and secondThread lock, only one instance of each can be run concurrently
  thread_create("Thread1",firstThread);
  thread_create("Thread2",firstThread);
  thread_create("Thread3",secondThread);
  thread_create("Thread4",secondThread);

  thread_create("Thread5",firstThread);
  thread_create("Thread6",firstThread);
  thread_create("Thread7",secondThread);
  thread_create("Thread8",secondThread);

  thread_create("Thread9",firstThread);
  thread_create("Thread10",firstThread);
  thread_create("Thread11",secondThread);
  thread_create("Thread12",secondThread);

  thread_create("Thread13",firstThread);
  thread_create("Thread14",firstThread);
  thread_create("Thread15",secondThread);
  thread_create("Thread16",secondThread);


  thread_create("Thread17",firstThread);
  thread_create("Thread18",firstThread);
  thread_create("Thread19",secondThread);
  thread_create("Thread20",secondThread);

  thread_create("Thread21",firstThread);
  thread_create("Thread22",firstThread);
  thread_create("Thread23",secondThread);
  thread_create("Thread24",secondThread);

  thread_create("Thread25",firstThread);
  thread_create("Thread26",firstThread);
  thread_create("Thread27",secondThread);
  thread_create("Thread28",secondThread);

  thread_create("Thread29",firstThread);
  thread_create("Thread30",firstThread);
  thread_create("Thread31",secondThread);
  thread_create("Thread32",secondThread);

  thread_create("Thread33",firstThread);
  thread_create("Thread34",firstThread);
  thread_create("Thread35",secondThread);
  thread_create("Thread36",secondThread);

  thread_create("Thread37",firstThread);
  thread_create("Thread38",firstThread);
  thread_create("Thread39",secondThread);
  thread_create("Thread40",secondThread);

  thread_create("Thread41",firstThread);
  thread_create("Thread42",firstThread);
  thread_create("Thread43",secondThread);
  thread_create("Thread44",secondThread);

  thread_create("Thread45",firstThread);
  thread_create("Thread46",firstThread);
  thread_create("Thread47",secondThread);
  thread_create("Thread48",secondThread);

  thread_create("Thread49",firstThread);
  thread_create("Thread50",firstThread);
  thread_create("Thread51",secondThread);
  thread_create("Thread52",secondThread);

  thread_create("Thread53",firstThread);
  thread_create("Thread54",firstThread);
  thread_create("Thread55",secondThread);
  thread_create("Thread56",secondThread);


  thread_create("Thread57",firstThread);
  thread_create("Thread58",firstThread);
  thread_create("Thread59",secondThread);
  thread_create("Thread60",secondThread);
  
  thread_schedule();
  return 0;
}

