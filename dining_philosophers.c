

void sys_putdown_chopsticks(void)
{
 
  int phil_num;

  if(argint(0, &phil_num) < 0)
    return -1;
  
  sem_acquire(MUTEX);

  // state that thinking
  state[phil_num] = THINKING;

  printf("Philosopher %d putting fork %d and %d down\n",
          phil_num + 1, LEFT + 1, phil_num + 1);
  printf("Philosopher %d is thinking\n", phil_num + 1);

  test(LEFT);
  test(RIGHT);

  sem_release(MUTEX);
}