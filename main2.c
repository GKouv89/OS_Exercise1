#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
	pid_t proc_id_1, proc_id_2, proc_id_3; 
	proc_id_1 = fork();
	if(proc_id_1 == 0){ // child process
		char* encExec = "enc/enc";
		execlp(encExec, encExec, NULL);
	}else{
        proc_id_2 = fork();
        if(proc_id_2 == 0){ // child process
            char* pExec = "p/p";
            execlp(pExec, pExec, NULL);
        }else{
            proc_id_3 = fork();
            if(proc_id_3 == 0){
                char* chanExec = "chan/chan";
                execlp(chanExec, chanExec, NULL);
            }
        }
	}
    printf("Done with fork/exec combos.\n");
    if(proc_id_1 && proc_id_2 && proc_id_3){ // parent process
        waitpid(proc_id_1, NULL, 0);
        waitpid(proc_id_2, NULL, 0);
        waitpid(proc_id_3, NULL, 0);
    }
    printf("ENC1, ENC2, CHAN, P1, P2 finished.\n");
}
