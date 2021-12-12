#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, const char* argv[]){

  int pid, queue_level;
  if(argc < 3) { printf(2, "pid and queue_level args must be provided.\n"); return -1; }  
  pid = atoi(argv[1]);
  queue_level = atoi(argv[2]);
  if(queue_level < 1 || queue_level > 3) { printf(2, "queue_level must be between 1 to 3.\n"); return -1; }
  if(set_proc_queue(pid, queue_level) > -1){
    printf(2, "queue level of process pid=%d changed to level=%d.\n", pid, queue_level);
  }
  else {
    printf(2, "task faild.\n");
  }
  return 0;
}