#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main(){
    
    int pid = 0;
    init_semaphores();
    
    int phil_pides[5];
    char* argv[2];
    strcpy(argv[0], "philosopher");
    for(int i = 0; i < 5; i++){
        pid = fork();
        if(pid > 0){ 
            phil_pides[i] = pid;
            continue;
        }
        else if (pid == 0) {
            itoa(i, argv[1], 10);
            exec(argv[0], argv);
        }
        else { printf(1, "Bat thing happend!"); exit(); }

    }
    sleep(3000);
    for(int i = 0; i < 5; i++){
        kill(phil_pides[i]);
    }
    exit();
}
