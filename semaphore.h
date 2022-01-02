#include "spinlock.h"

#define N_SEM 6

int sem_init(int, int);
int sem_acquire(int);
int sem_release(int);

struct semaphore
{
	int counter;
	struct spinlock lk;
	int cv;
};

