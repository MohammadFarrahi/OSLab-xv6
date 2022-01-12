#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char *argv[])
{

    int cnt = get_free_pages_count();
    printf(2, "\nFree Pages Count: %d\n\n", cnt);
    exit();

}