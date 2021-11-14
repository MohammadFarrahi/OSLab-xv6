#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


int main(int argc, char *argv[])
{
    int pid;

    if((pid = fork()) == 0){
        printf(1, "processA pid : %d\n", getpid());
        printf(1, "parent of process A pid : %d\n", get_parent_pid());
        sleep(3000);
        printf(1, "processA pid : %d\n", getpid());
        printf(1, "parent of process A pid : %d\n", get_parent_pid());
        printf(1, "child A is exiting...\n");
    }
    else {
        wait();
        printf(1, "parent of A is exiting...\n");
    }
    exit();
}