#include "types.h"
#include "stat.h"
#include "fs.h"
#include "defs.h"
#include "fcntl.h"
#include "x86.h"
#include "semaphore.h"





int
sem_init(int i, int v)
{
  sem[i].counter = v;
  sem[i].cv = 0;
  initlock(&(sem[i].lk), "SEM_LOCK");
  return 0;
}

int
sem_release(int i)
{
  acquire(&(sem[i].lk));
  sem[i].counter++;
  wakeup(&(sem[i].cv));
  release(&(sem[i].lk));
  return 0;
}

int
sem_acquire(int i)
{
  acquire(&(sem[i].lk));
  sem[i].counter--;

  if (sem[i].counter < 0)
  	sleep(&(sem[i].cv), &(sem[i].lk));

  release(&(sem[i].lk));
  return 0;
}
