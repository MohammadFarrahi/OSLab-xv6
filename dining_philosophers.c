#include "dining_philosophers.h"
#define LEFT (phil_num + 4) % N
#define RIGHT (phil_num + 1) % N

int phil_state[N];
struct semaphore mutex;


void sys_init_semaphores(void)
{
  sem_init(MUTEX, 1);

  for (int i = 0; i < N-1; i++)
    sem_init(i, 0);
}

void try_picking(int phil_num)
{
    if (phil_state[phil_num] == HUNGRY && phil_state[LEFT] != EATING && phil_state[RIGHT] != EATING) {
        phil_state[phil_num] = EATING;
 
        sleep(2);
 
        printf(1, "Philosopher %d takes fork %d and %d\n",
                      phil_num + 1, LEFT + 1, phil_num + 1);
 
        printf(1, "Philosopher %d is Eating\n", phil_num + 1);
 
        sem_release(phil_num);
    }
}

void sys_pickup_chopsticks(void)
{
    int phil_num;
    if(argint(0, &phil_num) < 0)
        return -1;

    sem_acquire(MUTEX);

        phil_state[phil_num] = HUNGRY;

        printf(1, "Philosopher %d is Hungry\n", phil_num + 1);
    
        try_picking(phil_num);

    sem_release(MUTEX);
 
    sem_acquire(phil_num);
 
    sleep(1);
}


void sys_putdown_chopsticks(void)
{
 
  int phil_num;

  if(argint(0, &phil_num) < 0)
    return -1;
  
  sem_acquire(MUTEX);

  phil_state[phil_num] = THINKING;

  printf(1, "Philosopher %d putting fork %d and %d down\n",
          phil_num + 1, LEFT + 1, phil_num + 1);
  printf(1, "Philosopher %d is thinking\n", phil_num + 1);

  try_picking(LEFT);
  try_picking(RIGHT);

  sem_release(MUTEX);
}

