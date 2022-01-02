#include "dining_philosphers.h"

int state[N];
int phil[N] = { 0, 1, 2, 3, 4 };
struct semaphore mutex;


void sys_pickup_chopsticks(void)
{
    int phil_num;
    if(argint(0, &phil_num) < 0)
        return -1;

    
    sem_acquire(&mutex);

        state[phil_num] = HUNGRY;

        printf("Philosopher %d is Hungry\n", phil_num + 1); //***log***
    
        // eat if neighbours are not eating
        test(phil_num);

    sem_release(&mutex);
 
    // if unable to eat wait to be signalled
    sem_acquire(&S[phil_num]);
 
    sleep(1);
}
