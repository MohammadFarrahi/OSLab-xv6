#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


int main(int argc, char *argv[]){
    
    int phil_num = atoi(argv[1]);
    while(1) {
        sleep(1); // thinking
        pickup_chopsticks(phil_num);
        sleep(1); // eating
        putdown_chopsticks(phil_num);  
    }
    exit();
}