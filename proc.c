#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
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

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  p->queue_num = LCFS;
  p->cycles = 1;
  p->waiting_time = 0;
  acquire(&tickslock);
  p->arrival_time = ticks;
  release(&tickslock);

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

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
  curproc->sz = sz;
  switchuvm(curproc);
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

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
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
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
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
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
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
check_aging()
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;

    if (p->waiting_time > AGING_CYCLE)
    {
        p->queue_num = (p->queue_num == RR) ? RR : p->queue_num-1;
        p->waiting_time = 0;
    }
  }
}

void
update_waiting_times()
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;

    p->waiting_time++;
  }
}


	struct proc* get_proc_from_rr_queue(void)
{
  struct proc *p;
  struct proc *testee;
  int proc_exists = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state != RUNNABLE || p->queue_num != RR)
        continue;
    if(proc_exists)  //for first proc in rr queue
    {
        if(p->arrival_time < testee->arrival_time)
          testee = p;
    }
    else         //for others in rr queue
    {
        testee = p;
        proc_exists = 1;
    }
  }
	  if(proc_exists) //if any proccess in rr exists
      return testee;
  return NULL;
}

struct proc* get_proc_from_lcfs_queue(void)
{
  struct proc *curr_proc;
  struct proc *lc_proc;
  int has_proc = 0;
  for(curr_proc = ptable.proc; curr_proc < &ptable.proc[NPROC]; curr_proc++){
    if(curr_proc->state != RUNNABLE || curr_proc->queue_num != LCFS)
        continue;
    if(has_proc) {
      if(curr_proc->arrival_time > lc_proc->arrival_time)
        lc_proc = curr_proc;
    }
    else {
      lc_proc = curr_proc;
      has_proc = 1;
    }
  }

  if(has_proc)
    return lc_proc;

  return NULL;
}


double calculate_mhrrn(int arrival_time, int executed_cycles_number, int hrrn_priority)
{
  // acquire(&tickslock);
  int current_time = ticks;
  // release(&tickslock);
  int waiting_time = current_time - arrival_time;
  double hrrn = (waiting_time + executed_cycles_number)*1.0/executed_cycles_number;
  double mhrrn = (hrrn + hrrn_priority)/2;
  return mhrrn;
}

struct proc* get_proc_from_mhrrn_queue(void)
{
  struct proc *current_proc;
  struct proc *proc_with_most_mhrrn = 0;
  double max_ratio = 0.0;

  for(current_proc = ptable.proc; current_proc < &ptable.proc[NPROC]; ++current_proc)
  {
    if(current_proc->state != RUNNABLE || current_proc->queue_num != MHRRN)
      continue;

    double current_ratio = calculate_mhrrn(current_proc->arrival_time, current_proc->cycles, current_proc->mhrrn_priority);

    if (current_ratio > max_ratio)
    {
      max_ratio = current_ratio;
      proc_with_most_mhrrn = current_proc;
    }
  }
  return proc_with_most_mhrrn;
}

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = NULL;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);
    p = get_proc_from_rr_queue();

    if(p == NULL)
      p = get_proc_from_lcfs_queue();

    if(p == NULL)
      p = get_proc_from_mhrrn_queue();

    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    if(p != NULL)
    {
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        p->cycles += 1;
        update_waiting_times();
        p->waiting_time = 0;
        check_aging();
        swtch(&(c->scheduler), p->context);
        switchkvm();

        c->proc = NULL;
        if (p->state == RUNNABLE && p->queue_num == RR) {
          //acquire(&tickslock);
          p->arrival_time = ticks;
          //release(&tickslock);
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
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
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
  struct proc *p = myproc();

  if(p == 0)
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
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

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

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
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

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
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
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int
set_proc_queue(int pid, int dest_queue)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->queue_num = dest_queue;
      p->waiting_time = 0;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

char*
get_state_string(int state)
{
  switch(state) {
    case 0:
      return "UNUSED";
    case 1:
      return "EMBRYO";
    case 2:
      return "SLEEPING";
    case 3:
      return "RUNNABLE";
    case 4:
      return "RUNNING";
    case 5:
      return "ZOMBIE";
    default:
      return "";
  }
  return "";
}

char*
get_queue_string(int q)
{
  if (q == 1)
    return "RR";
  else if (q == 2)
    return "LCFS";
  else if (q == 3)
    return "MHRRN";
  return "";
}

int
get_int_len(int n)
{
  int len = 0;
  if (n == 0)
    return 1;
  while (n > 0) {
    n /= 10;
    len++;
  }
  return len;
}


void 
print_procs(void)
{
  struct proc *p;

  cprintf("name");
  for (int i = 0 ; i < 10 - 4 ; i++)
    cprintf(" ");
  
  cprintf("pid");
  for (int i = 0 ; i < 5 - 3 ; i++)
    cprintf(" ");

  cprintf("state");
  for (int i = 0 ; i < 10 - 5 ; i++)
    cprintf(" ");

  cprintf("queue");
  for (int i = 0 ; i < 10 - 5 ; i++)
    cprintf(" ");

  cprintf("priority");
  for (int i = 0; i < 10 - 8 ; i++)
    cprintf(" ");

  cprintf("arrival");
  for (int i = 0 ; i < 10 - 7 ; i++)
    cprintf(" ");

  cprintf("cycle");
  for (int i = 0 ; i < 8 - 5 ; i++)
    cprintf(" ");
  
  cprintf("HRRN");
  for (int i = 0 ; i < 8 - 4 ; i++)
    cprintf(" ");

  cprintf("\n");
  for (int i = 0 ; i < 86 ; i++)
    cprintf(".");
  cprintf("\n");



  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){ 
    if (p->state == UNUSED)
      continue;


    cprintf(p->name);
    for (int i = 0 ; i < 10 - strlen(p->name) ; i++)
      cprintf(" ");

    cprintf("%d", p->pid);
    for (int i = 0 ; i < 5 - get_int_len(p->pid) ; i++)
      cprintf(" ");

    cprintf(get_state_string(p->state));
    for (int i = 0 ; i < 10 - strlen(get_state_string(p->state)) ; i++)
      cprintf(" "); 

    cprintf(get_queue_string(p->queue_num));
    for (int i = 0 ; i < 10 - strlen(get_queue_string(p->queue_num)); i++)
      cprintf(" ");

    cprintf("%d", p->mhrrn_priority);
    for (int i = 0; i < 10 - get_int_len(p->mhrrn_priority); i++)
      cprintf(" ");

    cprintf("%d", p->arrival_time);
    for (int i = 0; i < 10 - get_int_len(p->arrival_time); i++)
      cprintf(" ");

    cprintf("%d", p->cycles);
    for (int i = 0; i < 8 - get_int_len(p->cycles); i++)
      cprintf(" ");
    int p_mhrrn = (int)calculate_mhrrn(p->arrival_time, p->cycles, p->mhrrn_priority);
    cprintf("%d", p_mhrrn);
    for (int i = 0; i < 8 - get_int_len(p_mhrrn); i++)
      cprintf(" ");

    cprintf("\n");
  }
  release(&ptable.lock);
}
