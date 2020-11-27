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

int main(int argc, char *argv[]){
    
    char *sh_mem1 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 0);
    if(sh_mem1 == NULL){
        fprintf(stderr, "Failed to create or attach to shared memory block in ENC1.\n");
    }

    char *sh_mem2 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 1);
    if(sh_mem2 == NULL){
        fprintf(stderr, "Failed to create shared memory block between ENC1 and CHAN.\n");
    }
    memset(sh_mem2, 0, BLOCK_SIZE);
    
    int direction = 1;
    int transmitted = 0;
    
    sem_t *mutex1 = sem_open(MUTEX1, 0);    
    sem_t *p1r = sem_open(P1_READ, 0);
    sem_t *p1w = sem_open(P1_WRITE, 0);
    sem_t *enc11r = sem_open(ENC11_READ, 0);
    sem_t *enc11w = sem_open(ENC11_WRITE, 0);
    
    sem_unlink(MUTEX2);
    sem_unlink(ENC12_READ);
    sem_unlink(ENC12_WRITE);
    sem_unlink(CHAN1_READ);
    sem_unlink(CHAN1_WRITE);
    
    sem_t *mutex2 = sem_open(MUTEX2, O_CREAT, 0660, 1);
    sem_t *enc12r = sem_open(ENC12_READ, O_CREAT, 0660, 0);
    sem_t *enc12w = sem_open(ENC12_WRITE, O_CREAT, 0660, 0);
    sem_t *chan1r = sem_open(CHAN1_READ, O_CREAT, 0660, 0);
    sem_t *chan1w = sem_open(CHAN1_WRITE, O_CREAT, 0660, 0);
    
    pid_t pid = fork();
    if(pid == 0){
        char *chan = "chan/chan";
        if(execlp(chan, chan, argv[1], NULL) == -1){
            perror("error code from execlp: ");
        }
    }else{ 
        msg *input = malloc(sizeof(msg));
        input->message = malloc(50*sizeof(char));
  
        while(1){
            if(direction == 1 && transmitted == 0){
                sem_wait(enc11r);
                sem_wait(mutex1);
                memcpy(input, sh_mem1, sizeof(int));
                
                if(memcmp(sh_mem1 + sizeof(int), "ALL_SET", input->length) == 0){
                    sem_post(mutex1);
                    sem_post(enc12w);
                    
                    sem_wait(enc12w);
                    sem_wait(mutex2);
                    direction = 2;
                    memcpy(sh_mem2, sh_mem1, sizeof(int) + input->length);
                    sem_post(mutex2);
                    sem_post(chan1r);
                }else{
                    memcpy(input->message, sh_mem1 + sizeof(int), input->length);
                    
                    sem_post(mutex1);
                    sem_post(enc12w);
                    
                    sem_wait(enc12w);
                    sem_wait(mutex2);
                    
                    for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
                        input->hash[i] = '\0';
                    }
                    MD5(input->message, input->length, input->hash);
                    memset(sh_mem2, 0, BLOCK_SIZE);
                    
                    memcpy(sh_mem2, input, sizeof(int));
                    memcpy(sh_mem2 + sizeof(int), input->message, input->length);
                    memcpy(sh_mem2 + sizeof(int) + input->length, input->hash, MD5_DIGEST_LENGTH*sizeof(char));
                    transmitted = 1;
                    sem_post(mutex2);
                    sem_post(chan1r);
                }
                
            }else if(direction == 1 && transmitted == 1){
                sem_wait(enc12r);
                sem_wait(mutex2);
                
                memcpy(input, sh_mem2, sizeof(int));
                    
                if(memcmp(sh_mem2 + sizeof(int), "TRANSMISSION_OK", input->length) == 0){
                    sem_post(mutex2);
                    sem_post(enc11w);
                    
                    sem_wait(enc11w);
                    sem_wait(mutex1);
                    memcpy(sh_mem1, sh_mem2, sizeof(int) + input->length);
                    transmitted = 0;
                    sem_post(mutex1);
                    sem_post(p1r);
                }
            }else if(direction == 2 && transmitted == 0){
                sem_wait(enc12r);
                sem_wait(mutex2);
                memcpy(input, sh_mem2, sizeof(int));
                
                if(memcmp(sh_mem2 + sizeof(int), "ALL_SET", input->length) == 0){
                    sem_post(mutex2);
                    sem_post(enc11w);
                    
                    sem_wait(enc11w);
                    sem_wait(mutex1);
                    direction = 1;
                    memcpy(sh_mem1, sh_mem2, sizeof(int) + input->length);
                    sem_post(mutex1);
                    sem_post(p1r);
                }else{
                    memcpy(input->message, sh_mem2 + sizeof(int), input->length);
                    memcpy(input->hash, sh_mem2 + sizeof(int) + input->length, MD5_DIGEST_LENGTH*sizeof(char));
                    
                    char new_hash[MD5_DIGEST_LENGTH];
                    MD5(input->message, input->length, new_hash);
                    
                    int wrong = 0;
                    for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
                        if(input->hash[i] != new_hash[i]){
                            wrong = 1;
                            break;
                        }
                    }
                    
                    if(wrong){
                        sem_post(mutex2);
                        input->length = strlen("RETRANSMIT");
                        memcpy(sh_mem2, input, sizeof(int));
                        memcpy(sh_mem2 + sizeof(int), "RETRANSMIT", input->length);
                        memcpy(sh_mem2 + sizeof(int) + input->length, input->hash, MD5_DIGEST_LENGTH*sizeof(char));
                        sem_post(chan1r);
                    }else{
                        sem_post(mutex2);
                        sem_post(enc11w);
                        
                        sem_wait(enc11w);
                        sem_wait(mutex1);
                        
                        memcpy(sh_mem1, sh_mem2, sizeof(int));
                        memcpy(sh_mem1 + sizeof(int), input->message, input->length);
                        transmitted = 1;
                        sem_post(mutex1);
                        sem_post(p1r);
                    }
                }
            }else if(direction == 2 && transmitted == 1){
                sem_wait(enc11r);
                sem_wait(mutex1);
                
                memcpy(input, sh_mem1, sizeof(int));
                    
                if(memcmp(sh_mem1 + sizeof(int), "TRANSMISSION_OK", input->length) == 0){
                    sem_post(mutex1);
                    sem_post(enc12w);
                    
                    sem_wait(enc12w);
                    sem_wait(mutex2);
                    memcpy(sh_mem2, sh_mem1, sizeof(int) + input->length);
                    transmitted = 0;
                    sem_post(mutex2);
                    sem_post(chan1r);
                }
            }
        }
        
        free(input->message);
        free(input);
        
        wait(NULL);
        if(detatch_from_block(sh_mem2) == -1){
            fprintf(stderr, "Failed to detach from ENC1->CHAN memory block.\n");
        }
        if(destroy_block(FIRST_FILE, 1) == -1) {
            fprintf(stderr, "Failed to delete ENC1->CHAN shared memory block.\n");
        }
        if(detatch_from_block(sh_mem1) == -1){
            fprintf(stderr, "Failed to detach from memory block.\n");
        }
    
        sem_close(mutex1);    
        sem_close(p1r);
        sem_close(p1w);
        sem_close(enc11r);
        sem_close(enc11w);
        
        sem_close(mutex2);
        sem_close(enc12r);
        sem_close(enc12w);
        sem_close(chan1r);
        sem_close(chan1w);
    }        
}
