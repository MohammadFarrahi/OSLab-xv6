#include "user.h"

int main(){
    
    int pid = 0;
    init_semaphores();
    
    char* argv[2];
    argv[0] = "philosopher";
    for(int i = 0; i < 5; i++){
        pid = fork();
        if(pid > 0) { continue; }
        else if (pid == 0) {
            itoa(i, argv[1], 10);
            exec(argv[0], argv);
        }
        else { printf(1, "Bat thing happend!"); exit(); }

    }

    exit();
}



