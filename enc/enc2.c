#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
/* ENC2 will not create the shared memory segment between herself and CHAN
    just attach herself to it.
    She will also only request the already opened for her semaphores,
    and only detach from the segment instead of deleting it. */

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
    char *sh_mem3 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 2);
    if(sh_mem3 == NULL){
        fprintf(stderr, "Failed to attach to shared memory block in ENC2.\n");
    }
    
    sem_t *mutex3 = sem_open(MUTEX3, 0);
    sem_t *enc21r = sem_open(ENC21_READ, 0);
    sem_t *enc21w = sem_open(ENC21_WRITE, 0);
    sem_t *chan2r = sem_open(CHAN2_READ, 0);
    sem_t *chan2w = sem_open(CHAN2_WRITE, 0);
    
    // READING FROM CHAN //  
    sem_wait(enc21r);
    sem_wait(mutex3);
    
    msg *input = malloc(sizeof(msg));
    input->message = malloc(50*sizeof(char));
    memcpy(input, sh_mem3, sizeof(int));
    
    memcpy(input->message, sh_mem3 + sizeof(int), input->length);
    memcpy(input->hash, sh_mem3 + sizeof(int) + input->length, MD5_DIGEST_LENGTH*sizeof(char));
    printf("ENC2 read %s from CHAN\n", input->message);
    printf("input->hash is: %s\n", (char*)input->hash);
    
    char new_hash[MD5_DIGEST_LENGTH];
    MD5(input->message, input->length, new_hash);
    printf ("new_hash is: %s\n", (char*)new_hash);
    
    int wrong = 0;
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
        if(input->hash[i] != new_hash[i]){
            wrong = 1;
            break;
        }
    }
    
    if(wrong){
        printf("Requesting retransmission from CHAN.\n");
    }
    
    sem_post(mutex3);
    sem_post(chan2r);
     
    free(input->message);
    free(input);
    
    if(detatch_from_block(sh_mem3) == -1){
        fprintf(stderr, "Failed to detach from ENC1->CHAN memory block.\n");
    }

    sem_close(mutex3);
    sem_close(enc21r);
    sem_close(enc21w);
    sem_close(chan2r);
    sem_close(chan2w);        
}
