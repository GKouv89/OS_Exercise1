#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
	printf("In enc_main.\n");
    pid_t proc_id;
    proc_id = fork();
    if(!proc_id){
       printf("ENC_2.\n");
    }else{
       printf("ENC_1. child_proc_id = %d.\n", proc_id);
        wait(NULL);
    }
}
