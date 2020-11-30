/* CHAN creates no other process, but it does create the memory segment it will shared
    with ENC2. 
    CHAN also has 10 semaphores available, symmetrically to ENC1. 2 of them are mutex sems
    that refer to the usage of the two memory segments it has access to, two of them
    refer to its ability to read from either of the two memory segments, two of them
    refer to the ability of ENC1 to read from or write to their shared memory segment
    and the last two refer to ENC2's ability to read or write to their shared memory segment.*/

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
    
    char *sh_mem3 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 2);
    if(sh_mem3 == NULL){
        fprintf(stderr, "Failed to create shared memory block between CHAN and ENC2.\n");
    }
    memset(sh_mem3, 0, BLOCK_SIZE);
    
    int direction = 1;
    // Transmitted's usage in CHAN is a bit different from the one in P1 and from the one in ENC1.
    // Here, transmitted indicates whether a transmission is necessary; when CHAN transmits
    // a message to either ENC1 or ENC2, it thinks that the transmission was OK and 
    // sets transmitted to 1, and waits to read TRANSMISSION_OK from either ENC1 or ENC2.
    // But, if the responsible for making the MD5 check ENC process requests retransmission,
    // then CHAN sets transmitted to 0, and tells itself to read the last correct
    // version of the message from the segment between itself and the other ENC process.
    int transmitted = 0; 
    int term = 0;
    
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
            
            if(memcmp(sh_mem2 + sizeof(int), "ALL_SET", input->length) == 0){
                sem_post(mutex2);
                sem_post(chan2w);
                
                sem_wait(chan2w);
                sem_wait(mutex3);
                
                direction = 2;
                memcpy(sh_mem3, sh_mem2, sizeof(int) + input->length);
                memset(sh_mem2, 0, BLOCK_SIZE);
                clear_buffer(input);
                
                sem_post(mutex3);
                sem_post(enc21r);
            }else if(memcmp(sh_mem2 + sizeof(int), "TERM", input->length) == 0){
                sem_post(mutex2);
                sem_post(chan2w);
                
                sem_wait(chan2w);
                sem_wait(mutex3);
                
                memcpy(sh_mem3, sh_mem2, sizeof(int) + input->length);
                term = 1;
            
                sem_post(mutex3);
                sem_post(enc21r);
            }else{
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

                // printf("CHAN morphed the message to: %s\n", input->message);
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
            }
            
            
        }else if(direction == 1 && transmitted == 1){
            sem_wait(chan2r);
            sem_wait(mutex3);
            
            memcpy(input, sh_mem3, sizeof(int));
            if(memcmp(sh_mem3 + sizeof(int), "RETRANSMIT", input->length) == 0){
                transmitted = 0;
                sem_post(mutex3);
                sem_post(chan1r);
            }else if(memcmp(sh_mem3 + sizeof(int), "TRANSMISSION_OK", input->length) == 0){
                sem_post(mutex3);
                sem_post(chan1w);
                
                sem_wait(chan1w);
                sem_wait(mutex2);
                memcpy(sh_mem2, sh_mem3, sizeof(int) + input->length);
                transmitted = 0;
                
                sem_post(mutex2);
                sem_post(enc12r);
            } 
        }else if(direction == 2 && transmitted == 0){
            sem_wait(chan2r);
            sem_wait(mutex3);            
            memcpy(input, sh_mem3, sizeof(int));
            
            if(memcmp(sh_mem3 + sizeof(int), "ALL_SET", input->length) == 0){
                sem_post(mutex3);
                sem_post(chan1w);
                
                sem_wait(chan1w);
                sem_wait(mutex2);
                
                direction = 1;
                memcpy(sh_mem2, sh_mem3, sizeof(int) + input->length);
                memset(sh_mem3, 0, BLOCK_SIZE);
                clear_buffer(input);
                
                sem_post(mutex2);
                sem_post(enc12r);
            }else if(memcmp(sh_mem3 + sizeof(int), "TERM", input->length) == 0){
                sem_post(mutex3);
                sem_post(chan1w);
                
                sem_wait(chan1w);
                sem_wait(mutex2);
                
                memcpy(sh_mem2, sh_mem3, sizeof(int) + input->length);
                term = 1;
                
                sem_post(mutex2);
                sem_post(enc12r);
            }else{
                memcpy(input->message, sh_mem3 + sizeof(int), input->length);
                memcpy(input->hash, sh_mem3 + sizeof(int) + input->length, MD5_DIGEST_LENGTH*sizeof(char));
                
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

                // printf("CHAN morphed the message to: %s\n", input->message);
                sem_post(mutex3);
                sem_post(chan1w);
                
                sem_wait(chan1w);
                sem_wait(mutex2);
                
                memcpy(sh_mem2, input, sizeof(int));
                memcpy(sh_mem2 + sizeof(int), input->message, input->length);
                memcpy(sh_mem2 + sizeof(int) + input->length, input->hash, MD5_DIGEST_LENGTH*sizeof(char));
                transmitted = 1;
                
                sem_post(mutex2);
                sem_post(enc12r);
            }
        }else{
            sem_wait(chan1r);
            sem_wait(mutex2);
            
            memcpy(input, sh_mem2, sizeof(int));
            if(memcmp(sh_mem2 + sizeof(int), "RETRANSMIT", input->length) == 0){
                transmitted = 0;
                sem_post(mutex2);
                sem_post(chan2r);
            }else if(memcmp(sh_mem2 + sizeof(int), "TRANSMISSION_OK", input->length) == 0){
                sem_post(mutex2);
                sem_post(chan2w);
                
                sem_wait(chan2w);
                sem_wait(mutex3);
                memcpy(sh_mem3, sh_mem2, sizeof(int) + input->length);
                transmitted = 0;
                
                sem_post(mutex3);
                sem_post(enc21r);
            } 
        }
        if(term){
            break;
        }
    }
    
    free(input->message);
    free(input);
        
    if(detach_from_block(sh_mem2) == -1){
        fprintf(stderr, "Failed to detach from ENC1->CHAN memory block (in CHAN).\n");
    }
    
    if(detach_from_block(sh_mem3) == -1){
        fprintf(stderr, "Failed to detach from CHAN->ENC2 memory block (in CHAN).\n");
    }
    
    if(destroy_block(FIRST_FILE, 2) == -1) {
        fprintf(stderr, "Failed to delete CHAN->ENC2 shared memory block (in CHAN).\n");
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