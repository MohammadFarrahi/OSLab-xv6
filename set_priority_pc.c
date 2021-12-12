#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char* argv[])
{
    int pid, priority;
    if(argc < 3)
    {
        cprintf("pid and priority not provided.\n");
        return -1;
    } 
    pid = atoi(argv[1]);
    priority = atoi(argv[2]);
    if (set_mhrrn_priority_pc(pid, priority) > -1)
        cprintf("priority of process with %d changed to %d.\n", pid, priority);
    else
        cprintf("something wrong happend.\ntask failed...\n");
    return 0;
}