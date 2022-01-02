#include "dining_philosophers.h"
#define LEFT (phil_num + 4) % N
#define RIGHT (phil_num + 1) % N

int phil_state[N];
struct semaphore mutex;


void sys_pickup_chopsticks(void)
{
    int phil_num;
    if(argint(0, &phil_num) < 0)
        return -1;

    sem_acquire(MUTEX);

        phil_state[phil_num] = HUNGRY;

        printf("Philosopher %d is Hungry\n", phil_num + 1);
    
        test(phil_num);

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

  printf("Philosopher %d putting fork %d and %d down\n",
          phil_num + 1, LEFT + 1, phil_num + 1);
  printf("Philosopher %d is thinking\n", phil_num + 1);

  test(LEFT);
  test(RIGHT);

  sem_release(MUTEX);
}

