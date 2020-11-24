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
    char *sh_mem1 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 0);
    if(sh_mem1 == NULL){
        fprintf(stderr, "Failed to create or attach to shared memory block in ENC1.\n");
    }
    
    char *sh_mem2 = attach_to_block(FIRST_FILE, BLOCK_SIZE, 1);
    if(sh_mem2 == NULL){
        fprintf(stderr, "Failed to create shared memory block between ENC1 and CHAN.\n");
    }
    
    sem_t *p1 = sem_open(P1_SEM, 0);
    sem_t *enc1 = sem_open(ENC1_CONS, 0);
    
    sem_t *enc12 = sem_open(ENC1_PROD, O_CREAT, 0660, 0);
    sem_t *chan1 = sem_open(CHAN_SEM, O_CREAT, 0660, 1);
    
    pid_t pid = fork();
    if(pid == 0){
        char *chan = "chan/chan";
        if(execlp(chan, chan, NULL) == -1){
            perror("error code from execlp: ");
        }
    }else{        
        sem_wait(p1);
        
        msg *input = malloc(sizeof(msg));
        input->message = malloc(50*sizeof(char));
        memcpy(input, sh_mem1, sizeof(int));
        
        memcpy(input->message, sh_mem1 + sizeof(int), input->length);
        printf("ENC1 read %s from P1.\n", input->message);
        
        sem_post(enc1);
        
        sem_wait(chan1);
        
        MD5(input->message, input->length, input->hash);
        printf("input->hash is: %s\n", (char*)input->hash);
        memset(sh_mem2, 0, BLOCK_SIZE);
        
        memcpy(sh_mem2, input, sizeof(int));
        memcpy(sh_mem2 + sizeof(int), input->message, input->length);
        memcpy(sh_mem2 + sizeof(int) + input->length, input->hash, MD5_DIGEST_LENGTH*sizeof(char));
        
        free(input->message);
        free(input);
        
        sem_post(enc12);
        
        if(detatch_from_block(sh_mem2) == -1){
            fprintf(stderr, "Failed to detach from ENC1->CHAN memory block.\n");
        }
        if(destroy_block(FIRST_FILE, 1) == -1) {
            fprintf(stderr, "Failed to delete ENC1->CHAN shared memory block.\n");
        }
        if(detatch_from_block(sh_mem1) == -1){
            fprintf(stderr, "Failed to detach from memory block.\n");
        }
        sem_close(p1);
        sem_close(enc1);
        sem_close(enc12);
        sem_close(chan1);
    }        
}
