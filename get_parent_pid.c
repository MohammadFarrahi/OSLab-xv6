#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    printf(2, "parent pid : %d\n", get_parent_pid());
    exit();
}