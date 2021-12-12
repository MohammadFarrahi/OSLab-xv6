#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define NPRCS 10

int main(int argc, char const *argv[])
{
	int pid, wpid;
	for (int i = 0; i < NPRCS; i++)
	{
		pid = fork();
		if(pid < 0) {
			printf(2, "fork failed!\n");
		}
		if(pid == 0) { 
			// printf(1, "\n******foo process : child with pid=%d created!\n", getpid());
			break;
		}

	}
	if(pid == 0) {
		int z = 1;
		for(long int j = 0; j < 2000000000; j+=1)
			z += (j + 1);
		printf (2, "", z);
	}
	else {
		while((wpid = wait()) >= 0){
			printf(1, "child pid=%d finished!\n", wpid);
		}
	}
	exit();
	return 0;
}
