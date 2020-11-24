#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#include "../shared/shared_memory.h"
#include "../shared/shared_semaphores.h"
#include "../shared/message_format.h"

int main(){
    char *sh_mem2 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 1);
    if(sh_mem2 == NULL){
        fprintf(stderr, "Failed to create shared memory block between ENC1 and CHAN.\n");
    }

    sem_t *enc12 = sem_open(ENC1_PROD, 0);
    sem_t *chan1 = sem_open(CHAN_SEM, 0);
    
    sem_wait(enc12);
    
    msg *input = malloc(sizeof(msg));
    input->message = malloc(50*sizeof(char));
    memcpy(input, sh_mem2, sizeof(int));
    
    memcpy(input->message, sh_mem2 + sizeof(int), input->length);
    memcpy(input->hash, sh_mem2 + sizeof(int) + input->length, MD5_DIGEST_LENGTH*sizeof(char));
    printf("CHAN read %s from ENC1\n", input->message);
    printf("input->hash is: %s\n", (char*)input->hash);
    free(input->message);
    free(input);
    
    sem_post(chan1);
    
    if(detatch_from_block(sh_mem2) == -1){
        fprintf(stderr, "Failed to detach from ENC1->CHAN memory block.\n");
    }

    sem_close(enc12);
    sem_close(chan1);
}