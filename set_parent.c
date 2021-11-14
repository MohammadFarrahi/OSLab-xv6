#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


int main(int argc, char *argv[])
{
    if(argc != 2){
        printf(2, "pid must be provied\n");
        exit();
    }
    int pid = atoi(argv[1]);
    printf(1, "setting process of pid=%d to be (unreal) parent of process pid=%d\n", getpid(), pid);
    if(set_process_parent(pid) > 0){
        printf(1, "successful setting.\n");
    }
    sleep(3000);
    exit();
}