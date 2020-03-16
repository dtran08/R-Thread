#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

//define locks
mutex_p lock1;
mutex_p lock2;


void firstThread(void) {

  lock_acquire(lock1); //begin crit selec
  printf(1, "Thread \'%s\' has started running\n",current_thread->name);

  int i;
  for (i = 0; i < 10; i++) {
    printf(1, "Thread \'%s\' at 0x%x\n", current_thread->name, (int) current_thread);
    thread_yield(); //yield at crit selec
  }
  printf(1, "Thread \'%s\' has finished; exititing\n",current_thread->name);
  
  current_thread->state = FREE;//reassign state
  lock_release(lock1);//end crit selec
  thread_schedule();
}



void secondThread(void) {
  lock_acquire(lock2);//begin crit selec
  printf(1, "Thread \'%s\' has started running\n",current_thread->name);
  
  int i;
  for (i = 0; i < 10; i++) {
    printf(1, "Thread \'%s\' at 0x%x\n", current_thread->name, (int) current_thread);
    thread_yield();//yield at crit selec
  }
  
  printf(1, "Thread \'%s\' has finished; exiting\n",current_thread->name);
  
  current_thread->state = FREE;//reassign state
  lock_release(lock2);//end crit selec
  thread_schedule();
}


int main(int argc, char *argv[]) {
  thread_init();
  mutex_t mutex1, mutex2;

  lock1 = &mutex1;//assign mutexes
  lock2 = &mutex2;

  init_lock(lock1);//init lock since queue
  init_lock(lock2);

  thread_create("Thread1",firstThread);
  thread_create("Thread2",firstThread);
  thread_create("Thread3",secondThread);
  thread_create("Thread4",secondThread);

  thread_schedule();
  return 0;
}

