/* P1_test shares a memory segment with ENC1_test */
/* First solution is to share one semaphore:
    since both are readers and writers at the same time,
    they just check to see whether the other one wrote something
    in the mem segment
    If they did, they print that out
    If not, and they have input, they write that
    If not and they don't have input, nothing happens */
/* Second solution is to share 3 semaphores, like in the bounded buffer implementation
    and have both processes contain the consumer AND the producer code */
/* Third solution involves 5 semaphores: mutex, responsible for the mem segment,
    P1_READ, indicating P1 should be expecting to read from the segment
    P1_WRITE, indicating P1 should be expecting to write to the segment
    ENC1_READ, and ENC1_WRITE, respectively.
    IF P1 ends up writing first to the memory segment, ENC1 should expect to read
    from the segment, and vice versa.
    To make this a bit easier, the messages will be predefined in buffers */

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



int main(){

    // Shared memory operations
    char *sh_mem = attach_to_block(FIRST_FILE, BLOCK_SIZE, 0);
    if(sh_mem == NULL){
        fprintf(stderr, "Failed to create or attach to shared memory block in P1.\n");
    }
        
    // Semaphore ops. P1 will create the semaphores and ENC1 will simply ask for them //
    sem_unlink(MUTEX1);
    sem_unlink(P1_READ);
    sem_unlink(P1_WRITE);
    sem_unlink(ENC1_READ);
    sem_unlink(ENC1_WRITE);
    
    sem_t *mutex = sem_open(MUTEX1, O_CREAT, 0660, 1);
    sem_t *p1r = sem_open(P1_READ, O_CREAT, 0660, 0); //At first, the memory segment should be empty
    // so both reading semaphores are 0 - we're not expecting any process to read from the segment at first
    sem_t *p1w = sem_open(P1_WRITE, O_CREAT, 0660, 1); //But both processes have 'equal' chance of writing
    // to the segment (really depends on which executable starts first)
    sem_t *enc1r = sem_open(ENC1_READ, O_CREAT, 0660, 0);
    sem_t *enc1w = sem_open(ENC1_WRITE, O_CREAT, 0660, 1);
    
    
    msg *input = malloc(sizeof(msg));
    input->message = malloc(50*sizeof(char));
    strcpy(input->message, "Got axes, need victims.\n");
    memset(sh_mem, 0, BLOCK_SIZE);

    /* P1 will write first */
    sem_wait(p1w);
    sem_wait(mutex);
    sem_wait(enc1w); // ENC1 SHOULD *NOT* BE EXPECTING TO WRITE TO THE MEMORY
    // AND INSTEAD SHOULD WAIT TO READ FROM IT BEFORE WRITING ITS OWN MESSAGE
    // printf("P1 produces\n");
    printf("Input: ");
    fgets(input->message, 50, stdin);
    strcpy(sh_mem, input->message);
    sem_post(mutex);
    sem_post(enc1r);
    
    int term = 0;

    while(1){
        // printf("IN P1 WHILE, BEFORE IF\n");
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
            sem_wait(p1w);
            sem_wait(mutex);
            // printf("P1 produces\n");
            printf("Input: ");
            fgets(input->message, 50, stdin);
            if(strcmp(input->message, "TERM\n") == 0){
                strcpy(sh_mem, "P1_TERM");
            }else{
                strcpy(sh_mem, input->message);
            }
            sem_post(mutex);
            sem_post(enc1r);
        }else{
            sem_wait(p1r);
            sem_wait(mutex);
            // printf("P1 consumes\n");
            if(strcmp(sh_mem, "ENC1_TERM") == 0){
                term = 1;
            }else{
                printf("%s", sh_mem);
            }
            memset(sh_mem, 0, BLOCK_SIZE);
            // Now, P1 must write
            sem_post(mutex);
            sem_post(p1w);
        }
        if(term){
            break;
        }
    }
    
    free(input->message);
    free(input);
        
    if(detatch_from_block(sh_mem) == -1){
        fprintf(stderr, "Failed to detach from memory block.\n");
    }
    if(destroy_block(FIRST_FILE, 0) == -1) {
        fprintf(stderr, "Failed to delete shared memory block.\n");
    }
    sem_close(mutex);
    sem_close(p1r);
    sem_close(p1w);
    sem_close(enc1r);
    sem_close(enc1w);
}