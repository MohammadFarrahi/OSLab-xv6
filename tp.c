#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"


int main() {

    set_proc_queue(10, 1);
    set_proc_queue(12, 1);
    set_proc_queue(14, 3);
    set_proc_queue(8, 3);
    set_proc_queue(9, 3);
    set_proc_queue(7, 1);

    set_mhrrn_priority_pc(14, 2000);
    set_mhrrn_priority_pc(9, 2000);
    print_procs();

    exit();
}