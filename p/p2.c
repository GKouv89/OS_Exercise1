/* P2 creates ENC2, with a shared memory segment between them */
/* P2 will read from ENC2 and print the final output if the checksum is correct
    in the first send operation. Then, it takes turns between reading from keyboard
    and writing to console, and so does P1.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#include "../shared/shared_memory.h"
#include "../shared/shared_semaphores.h"
#include "../shared/message_format.h"

int main(){

    // Shared memory operations
    char *sh_mem_fin = attach_to_block(FIRST_FILE, BLOCK_SIZE, 3);
    if(sh_mem_fin == NULL){
        fprintf(stderr, "Failed to create P2->ENC2 shared memory block (in P2).\n");
    }
    memset(sh_mem_fin, 0, BLOCK_SIZE);
    
    // The usage of direction and transmitted is symmetrical to P1.
    int direction = 1;
    int transmitted = 0;
    int term = 0;
    
    sem_unlink(MUTEX4);
    sem_unlink(ENC22_READ);
    sem_unlink(ENC22_WRITE);
    sem_unlink(P2_READ);
    sem_unlink(P2_WRITE);
    
    sem_t *mutex4 = sem_open(MUTEX4, O_CREAT, 0660, 1);
    sem_t *p2r = sem_open(P2_READ, O_CREAT, 0660, 0);
    sem_t *p2w = sem_open(P2_WRITE, O_CREAT, 0660, 0);
    sem_t *enc22r = sem_open(ENC22_READ, O_CREAT, 0660, 0);
    sem_t *enc22w = sem_open(ENC22_WRITE, O_CREAT, 0660, 0);
    
    
    pid_t pid = fork();
    if(pid == 0){
        char *enc2 = "enc/enc2";
        if(execlp(enc2, enc2, NULL) == -1){
            perror("error code from execlp: ");
        }
    }else{   
        
        msg *input = malloc(sizeof(msg));
        input->message = malloc(50*sizeof(char));
        
        while(1){
            if(direction == 1){
                sem_wait(p2r);
                sem_wait(mutex4);
                
                memcpy(input, sh_mem_fin, sizeof(int));
                if(memcmp(sh_mem_fin + sizeof(int), "ALL_SET", input->length) == 0){
                    sem_post(p2w);
                    
                    sem_wait(p2w);
                    direction = 2;
                    
                    clear_buffer(input);
                    
                    printf("Input: ");
                    fgets(input->message, BLOCK_SIZE, stdin);
                    input->message = strtok(input->message, "\n");
                    if(strcmp(input->message, "bye") == 0){
                        strcpy(input->message, "TERM");
                        term = 1;
                    }
                    input->length = strlen(input->message);
                    memcpy(sh_mem_fin, input, sizeof(int));
                    memcpy(sh_mem_fin + sizeof(int), input->message, input->length*sizeof(char));
                    transmitted = 1;
                    
                    sem_post(mutex4);
                    sem_post(enc22r);
                }else if(memcmp(sh_mem_fin + sizeof(int), "TERM", input->length) == 0){
                    term = 1;
                }else{
                    memcpy(input->message, sh_mem_fin + sizeof(int), input->length);
                
                    printf("Received: %s\n", input->message);
                    
                    input->length = strlen("TRANSMISSION_OK");
                    memcpy(sh_mem_fin, input, sizeof(int));
                    memcpy(sh_mem_fin + sizeof(int), "TRANSMISSION_OK", input->length);
                    
                    sem_post(mutex4);
                    sem_post(enc22r);
                }
            }else if(direction == 2 && transmitted == 1){
                sem_wait(p2r);
                sem_wait(mutex4);
                
                memcpy(input, sh_mem_fin, sizeof(int));
                if(memcmp(sh_mem_fin + sizeof(int), "TRANSMISSION_OK", input->length) == 0){
                    direction = 1;
                    transmitted = 0;
                    sem_post(mutex4);
                    sem_post(p2w);
                    
                    // ALL_SET MESSAGE TRANSMISSION
                    sem_wait(p2w);
                    sem_wait(mutex4);
                    
                    input->length = strlen("ALL_SET");
                    memcpy(sh_mem_fin, input, sizeof(int));
                    memcpy(sh_mem_fin + sizeof(int), "ALL_SET", input->length);
                    clear_buffer(input);
                    
                    sem_post(mutex4);
                    sem_post(enc22r);
                }
            }
            if(term){
                break;
            }
        }
        
        free(input->message);
        free(input);
        
        wait(NULL);
        if(detach_from_block(sh_mem_fin) == -1){
            fprintf(stderr, "Failed to detach from P2->ENC2 memory block (in P2).\n");
        }
        if(destroy_block(FIRST_FILE, 3) == -1) {
            fprintf(stderr, "Failed to delete P2->ENC2 shared memory block (in P2).\n");
        }
        
        sem_close(mutex4);
        sem_close(p2r);
        sem_close(p2w);
        sem_close(enc22r);
        sem_close(enc22w);
    }
}