#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char* argv[])
{
    int priority;
    if(argc < 2)
    {
        printf(2, "priority arg not provided.\n");
        return -1;
    } 
    priority = atoi(argv[1]);
    if (set_mhrrn_priority_os(priority) > -1)
        printf(2, "priority of all processes changed to %d.\n", priority);
    else
        printf(2, "something wrong happend\ntask failed...\n");
    return 0;
}