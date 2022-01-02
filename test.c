#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main(){
    
    int pid = 0;
    init_semaphores();
    char temp[20];
    int phil_pides[5];
    for(int i = 0; i < 5; i++){
        pid = fork();
        if(pid > 0){ 
            phil_pides[i] = pid;
            continue;
        }
        else if (pid == 0) {
            itoa(i, temp, 10);
            char* argv[] = {"philosopher", temp};
            exec(argv[0], argv);
        }
        else { printf(1, "Bat thing happend!"); exit(); }

    }
    sleep(50);
    for(int i = 0; i < 5; i++){
        kill(phil_pides[i]);
    }
    exit();
}
