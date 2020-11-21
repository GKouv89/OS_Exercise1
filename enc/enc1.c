#include <stdio.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "../p/common_keys1.h"

int main(){
    int shm_key = ftok(".", 'a');
    int shm_id = shmget((key_t) shm_key, MESSAGE_SIZE*sizeof(char) + sizeof(int), 0666 | IPC_CREAT);
    char *sh_mem1 = shmat(shm_id, NULL, 0);
    
    int mutex_key = ftok(".", 'b');
    int empty_key = ftok(".", 'c');
    int full_key = ftok(".", 'd');
    
    printf("HEY LOOK MA I MADE IT\n");
    
    int mutex = semget((key_t) mutex_key, 1, 0600 | IPC_CREAT);
    int empty = semget((key_t) empty_key, 1, 0600 | IPC_CREAT);
    int full = semget((key_t) full_key, 1, 0600 | IPC_CREAT);
    
    if (shmdt(sh_mem1) == -1) {
        fprintf(stderr, "shmdt in enc1 failed\n");
    }
}