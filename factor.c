#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

void factor(char* n) {
    int fd;
    unlink("factor_result.txt");
    fd = open("factor_result.txt", O_CREATE | O_RDWR);
    int num = atoi(n);
    char* buf = "";
    for(int i = 1; i <= num; ++i) {
        if (num % i == 0) {
            itoa(i, buf, 10);
            write(fd, buf, strlen(buf));
            write(fd, " ", 1);
            buf = "";
        }
    }
    write(fd, "\n", 1);
    close(fd);
}

int main(int argc, char *argv[])
{
    
    factor(argv[1]);
    exit();
}