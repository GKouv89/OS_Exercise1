#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
	printf("In p_main.\n");
    pid_t proc_id;
    proc_id = fork();
    if(!proc_id){
        printf("P2.\n");
    }else{
        printf("P1. child_proc_id = %d.\n", proc_id);
        wait(NULL);
    }
    
    // SOME TESTING ON PROMPT APPEARANCE ORDER
    // IF THE TWO P PROCESSES WERE CREATED THROUGH MAIN FUNCTION
    
    // pid_t curr_id = getpid();
    // printf("Hi, I'm process no. %d.\n", curr_id);
    // if(!proc_id){
        // printf("2.\n");
    // }else{
        // printf("1.\n");
    // }
    // printf("Give me your input: ");
    // printf("\n");
    // if(proc_id){
        // wait(NULL);
    // }
}
