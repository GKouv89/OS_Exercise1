/* P1 creates ENC1, with a shared memory segment between them */
/* They also share 3 semaphores; mutex, empty, full */
/* The latter two are counting semaphores, and are initialized as zero */
/* The former one is a binary semaphore, and is initialized as one */

#include <stdio.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#include "common_keys1.h"
// #define MESSAGE_SIZE 50

int main(){
    int shm_key = ftok(".", 'a');
    int shm_id = shmget((key_t) shm_key, MESSAGE_SIZE*sizeof(char) + sizeof(int), 0666 | IPC_CREAT);
    char *sh_mem1 = shmat(shm_id, NULL, 0);
    
    int mutex_key = ftok(".", 'b');
    int empty_key = ftok(".", 'c');
    int full_key = ftok(".", 'd');
    
    int mutex = semget((key_t) mutex_key, 1, 0600 | IPC_CREAT);
    int empty = semget((key_t) empty_key, 1, 0600 | IPC_CREAT);
    int full = semget((key_t) full_key, 1, 0600 | IPC_CREAT);
    
    pid_t pid = fork();
    if(pid == 0){
        char *enc1 = "enc/enc1";
        if(execlp(enc1, enc1, NULL) == -1){
            perror("error code from execlp: ");
        }
    }else{
        wait(NULL);
        if (shmdt(sh_mem1) == -1) {
            fprintf(stderr, "shmdt in p1 failed\n");
        }
        if(shmctl(shm_id, IPC_RMID, 0) == -1) {
            fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        }
        if (semctl(mutex, 0, IPC_RMID) < 0) {
            fprintf(stderr, "Could not delete mutex semaphore\n");
        }
        if (semctl(empty, 0, IPC_RMID) < 0) {
            fprintf(stderr, "Could not delete empty semaphore\n");
        }
        if (semctl(full, 0, IPC_RMID) < 0) {
            fprintf(stderr, "Could not delete full semaphore\n");
        }
    }
}