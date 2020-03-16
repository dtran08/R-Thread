#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"



struct t_mutex_kthread {
  uint allocated;
  uint locked;
  struct spinlock lk;
  int tid;
};

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


struct {
  struct spinlock lock;
  struct t_mutex_kthread mutex_arr [MAX_MUTEXES];
} mtable;



static struct proc *initproc;

int nextpid = 1;
int nexttid = 1;


extern void forkret(void);
extern void trapret(void);

extern void acquire_ptable(){
  acquire(&ptable.lock);
}

extern void release_ptable(){
  release(&ptable.lock);
}
static void wakeup1(void *chan);


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&mtable.lock, "mtable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}


struct thread*
mythread(void){
  struct cpu *c;
  struct thread *t;
  pushcli();
  c = mycpu();
  t = c->thread;
  popcli();
  return t;
}



static struct thread*
allocthread(struct proc *p)
{
  struct thread* t;
  char *sp;

  acquire (&ptable.lock);

  for(t = p->threads; t < &p->threads[NTHREAD]; t++){
    if(t->state == T_ZOMBIE){
      kfree(t->kstack);
      t->kstack = 0;
      t->killed = 0;
      t->state = T_UNUSED;
      t->tid = 0;
    }

    if(t->state == T_UNUSED){
      goto foundThread;
    }

  }

  release(&ptable.lock);
  return 0;

  foundThread:
  t->state = T_EMBRYO;
  t->tid = nexttid++;
  t->killed = 0;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((t->kstack = kalloc()) == 0){
    t->state = T_UNUSED;
    return 0;
  }

  sp = t->kstack + KSTACKSIZE;

  // Leave room for trap frame.

  sp -= sizeof *t ->tf;
  t->tf = (struct trapframe *) sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *t->context;
  t->context = (struct context*)sp;
  memset(t->context, 0, sizeof *t->context);
  t->context->eip = (uint)forkret;

  return t;
}


//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;


  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  struct thread* t;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  t = allocthread(p);
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(t->tf, 0, sizeof(*t->tf));
  t->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  t->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  t->tf->es = t->tf->ds;
  t->tf->ss = t->tf->ds;
  t->tf->eflags = FL_IF;
  t->tf->esp = PGSIZE;
  t->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  t->state = T_RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  acquire(&ptable.lock);
  curproc->sz = sz;
  release(&ptable.lock);
  
  switchuvm(curproc, mythread());

  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  struct thread *curthread = mythread();

  struct thread *t;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  t = allocthread(np);

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(t->kstack);
    t->kstack = 0;
    t->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *t->tf = *curthread->tf;

  // Clear %eax so that fork returns 0 in the child.
  t->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  t->state = T_RUNNABLE;

  release(&ptable.lock);

  return pid;
}


void
exit_thread(void)
{
  struct thread *t;
  struct thread *curthread = mythread();
  struct proc *curproc = myproc();

  // check if curThread is lastThread of curproc
  int lastThread = 1; 

  for(t = curproc->threads; t < &curproc->threads[NTHREAD]; t++){
    if(t != curthread && t->state != T_UNUSED && t->state != T_ZOMBIE){
      lastThread = 0;
      break;
    }
  }

  // if last thread and curproc don't exit
  if(lastThread && curproc->state != ZOMBIE){
    release(&ptable.lock);
    exit();
  }

  curthread->state = T_ZOMBIE;
  
  // wakeup other threads so they can die
  for(t = curproc->threads; t < &curproc->threads[NTHREAD]; t++){
    if(t->state == T_SLEEPING && t->chan == curthread){
      t->state = T_RUNNABLE;
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct thread *curthread = mythread();
  struct proc *p;
  struct thread *t;

  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  

  for(t = curproc->threads; t < &curproc ->threads[NTHREAD]; t++){
    if(t->state != T_UNUSED){
      t->killed = 1;
    }
  }

  areOtherThreadsDead:
    // wake up other threads so they can die
  for(t = curproc->threads; t < &curproc->threads[NTHREAD]; t++){
    if(t->state == T_SLEEPING){
      t->state = T_RUNNABLE;
    }
  }

  // check if all threads are dead, and then continue
  for(t = curproc->threads; t < &curproc->threads[NTHREAD]; t++){
    if(t!=curthread && t->state != T_UNUSED && t->state != T_ZOMBIE){
      goto areOtherThreadsDead;
    }
  }
      
    acquire(&ptable.lock);
  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  exit_thread();
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  struct thread *t;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.

        for(t=p->threads; t < &p->threads[NTHREAD]; t++){
          if(t->state != T_UNUSED){
            kfree(t->kstack);
            t->kstack = 0;
            t->killed = 0;
            t->state = T_UNUSED;
          }
        }
        pid = p->pid;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct thread *t;
  struct cpu *c = mycpu();
  c->proc = 0;
  c->thread = 0;
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      for(t = p->threads; t< &p->threads[NTHREAD]; t++){
        if(t->state == T_RUNNABLE){
          // Switch to chosen process.  It is the process's job
          // to release ptable.lock and then reacquire it
          // before jumping back to us.
          c->proc = p;
          c->thread = t;
          switchuvm(p,t);
          t->state = T_RUNNING;

          swtch(&(c->scheduler), t->context);
          switchkvm();

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
          c->thread = 0;
        }
      }
      
      
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct thread *t = mythread();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(t->state == T_RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&t->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  mythread()->state = T_RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct thread *t = mythread();
  
  if(t == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  t->chan = chan;
  t->state = T_SLEEPING;

  sched();

  // Tidy up.
  t->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;
  struct thread *t;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    for(t = p->threads; t< &p->threads[NTHREAD]; t++){
      if(t->state == T_SLEEPING && t->chan == chan){
        t->state = T_RUNNABLE;
      }
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;
  struct thread *t;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state != UNUSED){
        for(t = p->threads; t < &p->threads[NTHREAD]; t++){
          t->killed = 1;

          if(t->state == T_SLEEPING)
            t->state = T_RUNNABLE;
        }
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  //[SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  //[RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  //int i;
  struct proc *p;
  char *state;
  //uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
  /*  if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }*/
    cprintf("\n");
  }
}

// Basic Kthread function implementations
int 
create_kthread(void(*start_function)(), void* stack)
{
  struct thread *t;
  struct thread *curthread = mythread();

  t = allocthread(myproc());

  // if allocatting the thread worked
  if(t == 0){
    return -1;
  }

  acquire(&ptable.lock);

  // pass data to new thread

  *t->tf = *curthread->tf;
  t->tf->esp = (uint) (stack);
  t->tf->eip = (uint) start_function;

  t->state = T_RUNNABLE;

  release(&ptable.lock);

  return t->tid;
}


int 
id_kthread()
{
  if(mythread() == 0){
    return -1;
  }

  return mythread()->tid;
  
}

void
exit_kthread()
{
    acquire(&ptable.lock);
  exit_thread();

  // Jump into the scheduler, never to return.
  sched();
  panic("kthread exit: should never get to here.");
}

int
join_kthread(int id_thread)
{
  struct thread *t;
  acquire(&ptable.lock);

  for(t = myproc()->threads; t < &myproc()->threads[NTHREAD]; t++){
    if(t->tid == id_thread){
      goto foundThreadID;
    }
  }

  release(&ptable.lock);
  return -1;

  foundThreadID:

  while(t->state != T_ZOMBIE && t->state!=T_UNUSED)
    sleep(t, &ptable.lock);

  if(t->state == T_ZOMBIE){
    kfree(t->kstack);
    t->kstack = 0;
    t->killed = 0;
    t->state = T_UNUSED;
    t->tid = 0;
  }

  release(&ptable.lock);

  return 0;
}


int 
mutex_alloc_kthread()
{
  acquire(&mtable.lock);
  struct t_mutex_kthread *mute;
  int i;
  for(i = 0; i < MAX_MUTEXES; i++){
    mute = &mtable.mutex_arr[i];
    if(mute->allocated == 0){
      goto foundMutex;
    }
  }
  release(&mtable.lock);
  return -1;

  foundMutex:
    initlock(&mute ->lk, "mutex lock");
    mute->locked = 0;
    mute->tid = 0;
    mute->allocated = 1;
    release(&mtable.lock);
    return i;
}
/*  de-allocates mutex obj. which is not needed
    returns 0 w/ success and -1 w/ failure
    e.g. failure = mutex locked

*/
int mutex_dealloc_kthread(int id_mutex){
acquire(&mtable.lock);  
  struct t_mutex_kthread *mute = &mtable.mutex_arr[id_mutex];
  

if(mute == 0 || mute->allocated == 0 || mute->locked || mute->tid != 0){
  release(&mtable.lock);
  return -1;
}
mute->tid = 0;
mute->allocated = 0;
  
  release(&mtable.lock);

  return 0;
}

/* locks the mutex specified by the argument mutex id 
   if mutex already locked by another thread, call will block 
   calling thread until mutex unlocked
*/
int mutex_lock_kthread(int id_mutex){
  struct t_mutex_kthread *mute = &mtable.mutex_arr[id_mutex];
  if(mute->allocated == 0){
    return -1;
  }

  acquire(&mute->lk);
  while(mute->locked){
    sleep(mute, &mute->lk);
  }
  mute->locked = 1;
  mute->tid = mythread()->tid;
  release(&mute->lk);

  return 0;
}
/*  unlocks the mutex by the arg. id_mutex if called by 
    owning thread
    unlocking mutex --> one of the threads are waiting to wake up
    and acquire it 
*/
int mutex_unlock_kthread(int id_mutex){
  struct t_mutex_kthread *mute = &mtable.mutex_arr[id_mutex];

  if(mute->allocated == 0){
    return -1;
  }
  if(mute->tid != mythread()->tid){
    return -1;
  }

  acquire(&mute->lk);
  mute->locked = 0;
  mute->tid = 0;
  wakeup(mute);
  release(&mute->lk);
  return 0;
}