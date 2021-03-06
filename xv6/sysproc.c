#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_create_kthread(void)
{
  void(*start_function)();
  void* stack;

  if(argptr(0, (void*)&start_function, sizeof(*start_function)) < 0)
    return -1;
  if(argptr(1, (void*)&stack, sizeof(*stack)) < 0)
    return -1;

  return create_kthread(start_function, stack);
}

int 
sys_id_kthread(void)
{
  return id_kthread();
}

int 
sys_exit_kthread(void)
{
  exit_kthread();
  return 0;
}

int
sys_join_kthread(void)
{
  int tid;

  if(argint(0, &tid) < 0){
    return -1;
  }

  return join_kthread(tid);
}

int 
sys_mutex_alloc_kthread(void)
{
  return mutex_alloc_kthread();
}

int
sys_mutex_dealloc_kthread(void)
{
  int id_mutex;
  if(argint(0, &id_mutex) < 0){
    return -1;
  }
  return mutex_dealloc_kthread(id_mutex);
}

int
sys_mutex_lock_kthread(void)
{
  int id_mutex;
  if(argint(0, &id_mutex) < 0){
    return -1;
  }
  return mutex_lock_kthread(id_mutex);
}

int 
sys_mutex_unlock_kthread(void){
  int id_mutex;
  if(argint(0, &id_mutex) < 0){
    return -1;
  }
  return mutex_unlock_kthread(id_mutex);
}