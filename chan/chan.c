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

void first_transmission();

int main(int argc, char *argv[]){
    char *sh_mem2 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 1);
    time_t t;
    srand((unsigned) time(&t));
    if(sh_mem2 == NULL){
        fprintf(stderr, "Failed to create shared memory block between ENC1 and CHAN.\n");
    }
    
    char *sh_mem3 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 2);
    if(sh_mem3 == NULL){
        fprintf(stderr, "Failed to create shared memory block between CHAN and ENC2.\n");
    }
    memset(sh_mem3, 0, BLOCK_SIZE);
    
    int direction = 1;
    int transmitted = 0; // Zero right before a transmission attempt, 1 afterwards
    
    sem_t *mutex2 = sem_open(MUTEX2, 0);   
    sem_t *enc12r = sem_open(ENC12_READ, 0);
    sem_t *enc12w = sem_open(ENC12_WRITE, 0);
    sem_t *chan1r = sem_open(CHAN1_READ, 0);
    sem_t *chan1w = sem_open(CHAN1_WRITE, 0);
    
    sem_unlink(MUTEX3);
    sem_unlink(CHAN2_READ);
    sem_unlink(CHAN2_WRITE);
    sem_unlink(ENC21_READ);
    sem_unlink(ENC21_WRITE);
    
    sem_t *mutex3 = sem_open(MUTEX3, O_CREAT, 0660, 1);
    sem_t *enc21r = sem_open(ENC21_READ, O_CREAT, 0660, 0);
    sem_t *enc21w = sem_open(ENC21_WRITE, O_CREAT, 0660, 0);
    sem_t *chan2r = sem_open(CHAN2_READ, O_CREAT, 0660, 0);
    sem_t *chan2w = sem_open(CHAN2_WRITE, O_CREAT, 0660, 0);
    
    msg *input = malloc(sizeof(msg));
    input->message = malloc(50*sizeof(char));
    
    while(1){
        if(direction == 1 && transmitted == 0){
            sem_wait(chan1r);
            sem_wait(mutex2);            
            memcpy(input, sh_mem2, sizeof(int));
            
            memcpy(input->message, sh_mem2 + sizeof(int), input->length);
            memcpy(input->hash, sh_mem2 + sizeof(int) + input->length, MD5_DIGEST_LENGTH*sizeof(char));
            
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
            sem_post(mutex2);
            sem_post(chan2w);
            
            sem_wait(chan2w);
            sem_wait(mutex3);
            
            memcpy(sh_mem3, input, sizeof(int));
            memcpy(sh_mem3 + sizeof(int), input->message, input->length);
            memcpy(sh_mem3 + sizeof(int) + input->length, input->hash, MD5_DIGEST_LENGTH*sizeof(char));
            transmitted = 1;
            
            sem_post(mutex3);
            sem_post(enc21r);
        }else if(direction == 1 && transmitted == 1){
            sem_wait(chan2r);
            sem_wait(mutex3);
            
            memcpy(input, sh_mem3, sizeof(int));
            if(memcmp(sh_mem3 + sizeof(int), "RETRANSMIT", input->length) == 0){
                transmitted = 0;
                sem_post(mutex3);
                sem_post(chan1r);
            }else if(memcmp(sh_mem3 + sizeof(int), "TRANSMISSION_OK", input->length) == 0){
                printf("CHAN SAID TRANS_OK\n");
                sem_post(mutex3);
                sem_post(chan1w);
                
                sem_wait(chan1w);
                sem_wait(mutex2);
                memcpy(sh_mem2, sh_mem3, sizeof(int) + input->length);
                
                sem_post(mutex2);
                sem_post(enc12r);
            } 
            
            
        }
    }
    
    free(input->message);
    free(input);
        
    if(detatch_from_block(sh_mem2) == -1){
        fprintf(stderr, "Failed to detach from ENC1->CHAN memory block.\n");
    }
    
    if(detatch_from_block(sh_mem3) == -1){
        fprintf(stderr, "Failed to detach from CHAN->ENC2 memory block.\n");
    }
    
    if(destroy_block(FIRST_FILE, 2) == -1) {
        fprintf(stderr, "Failed to delete CHAN->ENC2 shared memory block.\n");
    }

    sem_close(mutex2);
    sem_close(chan1r);
    sem_close(chan1w);
    sem_close(enc12r);
    sem_close(enc12w);

    sem_close(mutex3);
    sem_close(chan2r);
    sem_close(chan2w);
    sem_close(enc21r);
    sem_close(enc21w);
}