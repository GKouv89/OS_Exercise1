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
    
    sem_t *mutex1 = sem_open(MUTEX1, 0);    
    sem_t *p1r = sem_open(P1_READ, 0);
    sem_t *p1w = sem_open(P1_WRITE, 0);
    sem_t *enc1r = sem_open(ENC1_READ, 0);
    sem_t *enc1w = sem_open(ENC1_WRITE, 0);
    
    sem_unlink(MUTEX2);
    sem_unlink(CHAN_READ);
    sem_unlink(CHAN_WRITE);
    
    sem_t *mutex2 = sem_open(MUTEX2, O_CREAT, 0660, 1);   
    sem_t *chanr = sem_open(CHAN_READ, O_CREAT, 0660, 0);
    sem_t *chanw = sem_open(CHAN_WRITE, O_CREAT, 0660, 0);
    
    pid_t pid = fork();
    if(pid == 0){
        char *chan = "chan/chan";
        if(execlp(chan, chan, argv[1], NULL) == -1){
            perror("error code from execlp: ");
        }
    }else{        
        sem_wait(enc1r);
        sem_wait(mutex1);
        msg *input = malloc(sizeof(msg));
        input->message = malloc(50*sizeof(char));
        memcpy(input, sh_mem1, sizeof(int));
        
        memcpy(input->message, sh_mem1 + sizeof(int), input->length);
        printf("ENC1 read %s from P1.\n", input->message);
        
        sem_post(mutex1);
        sem_post(enc1w);
        
        sem_wait(enc1w);
        sem_wait(mutex2);
        
        MD5(input->message, input->length, input->hash);
        printf("input->hash is: %s\n", (char*)input->hash);
        memset(sh_mem2, 0, BLOCK_SIZE);
        
        memcpy(sh_mem2, input, sizeof(int));
        memcpy(sh_mem2 + sizeof(int), input->message, input->length);
        memcpy(sh_mem2 + sizeof(int) + input->length, input->hash, MD5_DIGEST_LENGTH*sizeof(char));
        
        free(input->message);
        free(input);
        
        sem_post(mutex2);
        sem_post(chanr);
        
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
        sem_close(enc1r);
        sem_close(enc1w);
    }        
}
