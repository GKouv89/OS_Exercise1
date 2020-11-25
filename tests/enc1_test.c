/* ENC1_test shares a memory segment with P1_test */
/* First solution is to share one semaphore:
    since both are readers and writers at the same time,
    they just check to see whether the other one wrote something
    in the mem segment
    If they did, they print that out
    If not, and they have input, they write that
    If not and they don't have input, nothing happens */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

// #include "../p/common_keys1.h"
#include "../shared/shared_memory.h"
#include "../shared/shared_semaphores.h"
#include "../shared/message_format.h"

int main(){
    char *sh_mem = attach_to_block(FIRST_FILE, BLOCK_SIZE, 0);
    if(sh_mem == NULL){
        fprintf(stderr, "Failed to create or attach to shared memory block in ENC1.\n");
    }
    
    sem_t *mutex = sem_open(MUTEX1, 0);
    sem_t *p1r = sem_open(P1_READ, 0); //At first, the memory segment should be empty
    // so both reading semaphores are 0 - we're not expecting any process to read from the segment at first
    sem_t *p1w = sem_open(P1_WRITE, 0); //But both processes have 'equal' chance of writing
    // to the segment (really depends on which executable starts first)
    sem_t *enc1r = sem_open(ENC1_READ, 0);
    sem_t *enc1w = sem_open(ENC1_WRITE, 0);
    
    msg *input = malloc(sizeof(msg));
    input->message = malloc(50*sizeof(char));
    strcpy(input->message, "You shout and throw machetes? How quaint.\n");
    
    /* ENC1 WILL CONSUME FIRST */
    
    while(1){
        // printf("IN ENC1 WHILE, BEFORE IF\n");
        // int val;
        // sem_getvalue(mutex, &val);
        // printf("mutex = %d\n", val);
        // sem_getvalue(p1r, &val);
        // printf("p1_read = %d\n", val);
        // sem_getvalue(p1w, &val);
        // printf("p1_write = %d\n", val);
        // sem_getvalue(enc1r, &val);
        // printf("enc1_read = %d\n", val);
        // sem_getvalue(enc1w, &val);
        // printf("enc1_write = %d\n", val);
        
        if(strcmp(sh_mem, "\0") == 0){ //PRODUCES
            sem_wait(enc1w);
            sem_wait(mutex);
            // printf("ENC1 produces\n");
            strcpy(sh_mem, input->message);
            sem_post(mutex);
            sem_post(p1r);
        }else{
            sem_wait(enc1r);
            sem_wait(mutex);
            // printf("ENC1 consumes\n");
            printf("%s", sh_mem);
            memset(sh_mem, 0, BLOCK_SIZE);
            // Now, ENC1 must write
            sem_post(mutex);
            sem_post(enc1w);
        }
    }
    
    if(detatch_from_block(sh_mem) == -1){
        fprintf(stderr, "Failed to detach from memory block.\n");
    }
    sem_close(mutex);
    sem_close(p1r);
    sem_close(p1w);
    sem_close(enc1r);
    sem_close(enc1w);
}
