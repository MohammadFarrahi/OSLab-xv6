#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int main()
{
    int fd;
    if((fd = open("test.txt", O_CREATE|O_WRONLY)) < 0){
      printf(1, "creating text.txt file failed\n");
      exit();
    }
    char buffer1[100] = "This was pain in the neck or smh!";
    write(fd, buffer1, 100);
    close(fd);

    if((fd = open("test.txt", O_WRONLY)) < 0){
      printf(1, "opening text.txt file failed\n");
      exit();
    }
    char buffer2[100];
    read(fd, buffer2, 100);
    
    printf(1, "free pages before mmap: %d\n", get_free_pages_count());
    char* mapped_mem = (char*)mmap(0, 20, 0, 0, fd, 0);
    printf(1, "free pages after mmap: %d\n", get_free_pages_count());
    int pid = fork();
    if(pid == 0) {
        pid = fork();
        if (pid > 0) {
            mapped_mem[2] = 'a';
            mapped_mem[3] = 't';
            wait();
            printf(1, "pid: %d, first address: %s\n", getpid(), mapped_mem);
        }
        else {
            mapped_mem[2] = 'h';
            mapped_mem[3] = 'i';
            printf(1, "free pages: %d\n", get_free_pages_count());
            printf(1, "pid: %d, first address: %s\n", getpid(), mapped_mem);
        }
    }
    else {
        wait();
        printf(1, "free pages: %d\n", get_free_pages_count());
        printf(1, "pid: %d, first address: %s\n", getpid(), mapped_mem);

    }
    close(fd);
    exit();
}