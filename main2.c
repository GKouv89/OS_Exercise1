#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
	pid_t process_id; 
	process_id = fork();
	if(process_id == 0){ // child process
		char* testExecName = "execTest/main";
		execlp(testExecName, testExecName, NULL);
	}else{
		printf("Coming out of my cage...\n");
		wait(NULL);
	}
}
