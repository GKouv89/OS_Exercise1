#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#include "../shared/shared_memory.h"
#include "../shared/shared_semaphores.h"
#include "../shared/message_format.h"

int main(int argc, char *argv[]){
    char *sh_mem2 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 1);
    time_t t;
    srand((unsigned) time(&t));
    if(sh_mem2 == NULL){
        fprintf(stderr, "Failed to create shared memory block between ENC1 and CHAN.\n");
    }

    sem_t *mutex2 = sem_open(MUTEX2, 0);   
    sem_t *enc1r = sem_open(ENC1_READ, 0);
    sem_t *enc1w = sem_open(ENC1_WRITE, 0);
    sem_t *chanr = sem_open(CHAN_READ, 0);
    sem_t *chanw = sem_open(CHAN_WRITE, 0);

    sem_wait(chanr);
    sem_wait(mutex2);
    
    msg *input = malloc(sizeof(msg));
    input->message = malloc(50*sizeof(char));
    memcpy(input, sh_mem2, sizeof(int));
    
    memcpy(input->message, sh_mem2 + sizeof(int), input->length);
    memcpy(input->hash, sh_mem2 + sizeof(int) + input->length, MD5_DIGEST_LENGTH*sizeof(char));
    printf("CHAN read %s from ENC1\n", input->message);
    printf("input->hash is: %s\n", (char*)input->hash);
    
    float chance;
    char c;
    for(int i = 0; i < input->length; i++){
        chance = (rand() % 100);
        chance = chance/100; 
        if(chance < atof(argv[1])){
            memcpy(&c, input->message + i*sizeof(char), sizeof(char));
            c += 1;
            memcpy(input->message + i*sizeof(char), &c, sizeof(char));
        }
    }

    printf("CHAN morphed the message to: %s\n", input->message);
    free(input->message);
    free(input);
    
    sem_post(mutex2);
    sem_post(chanw);
    
    if(detatch_from_block(sh_mem2) == -1){
        fprintf(stderr, "Failed to detach from ENC1->CHAN memory block.\n");
    }

    sem_close(mutex2);
    sem_close(chanr);
    sem_close(chanw);
    sem_close(enc1r);
    sem_close(enc1w);
}