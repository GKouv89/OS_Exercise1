/* P1 creates ENC1, with a shared memory segment between them */
/* They also share 3 semaphores; mutex, empty, full */
/* The latter two are counting semaphores, and are initialized as zero */
/* The former one is a binary semaphore, and is initialized as one */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
// #include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

// #include "common_keys1.h"
#include "../shared/shared_memory.h"
#include "../shared/shared_semaphores.h"
#include "../shared/message_format.h"

int main(int argc, char *argv[]){
    
    if(argc != 2){
        printf("Correct program usage: ./main probability (must be a float between 0 and 1)\n");
        return 1;
    }

    // Shared memory operations
    char *sh_mem = attach_to_block(FIRST_FILE, BLOCK_SIZE, 0);
    if(sh_mem == NULL){
        fprintf(stderr, "Failed to create or attach to shared memory block in P1.\n");
    }
        
    // Semaphore ops. P1 will create the named semaphores, ENC1 will simply request them.
    sem_unlink(MUTEX1);
    sem_unlink(P1_READ);
    sem_unlink(P1_WRITE);
    sem_unlink(ENC1_READ);
    sem_unlink(ENC1_WRITE);
    
    // MUTEX is the semaphore responsible for checking who uses the shared mem segment
    sem_t *mutex = sem_open(MUTEX1, O_CREAT, 0600, 1);    
    sem_t *p1r = sem_open(P1_READ, O_CREAT, 0660, 0);
    sem_t *p1w = sem_open(P1_WRITE, O_CREAT, 0660, 1);
    sem_t *enc1r = sem_open(ENC1_READ, O_CREAT, 0660, 0);
    sem_t *enc1w = sem_open(ENC1_WRITE, O_CREAT, 0660, 0);
    
    pid_t pid = fork();
    if(pid == 0){
        char *enc1 = "enc/enc1";
        if(execlp(enc1, enc1, argv[1], NULL) == -1){
            perror("error code from execlp: ");
        }
    }else{        
        sem_wait(p1w);
        sem_wait(mutex);
        
        msg *input = malloc(sizeof(msg));
        input->message = malloc(50*sizeof(char));
        memset(sh_mem, 0, BLOCK_SIZE);
        
        printf("Input: ");
        fgets(input->message, BLOCK_SIZE, stdin);
        input->message = strtok(input->message, "\n");
        input->length = strlen(input->message);
        memcpy(sh_mem, input, sizeof(int));
        memcpy(sh_mem + sizeof(int), input->message, input->length*sizeof(char));
        
        free(input->message);
        free(input);
        
        sem_post(mutex);
        sem_post(enc1r);
        
        wait(NULL);
        if(detatch_from_block(sh_mem) == -1){
            fprintf(stderr, "Failed to detach from memory block.\n");
        }
        if(destroy_block(FIRST_FILE, 0) == -1) {
            fprintf(stderr, "Failed to delete shared memory block.\n");
        }
        sem_close(mutex);
        sem_close(p1r);
        sem_close(enc1r);
        sem_close(p1w);
        sem_close(enc1w);
    }
}