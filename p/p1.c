/* P1 creates ENC1, with a shared memory segment between them */
/* They also share 5 semaphores; mutex, which refers to the usage of the segment
    by only one process at a time, p1_read, which means it is p1's turn to
    read from the segment, p1_write, which means it is p1's turn to
    write to the segment and enc11_read, and enc11_write, which will
    be explained in enc1.c
    */
/* We initialize p1_write as 1, because we accept that it is p1 that will ask
    for keyboard input first, and therefore access the shared memory segment
    first, too. Mutex is also initialized as 1; everything else is initialized as 0.*/

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

int main(int argc, char *argv[]){
    
    if(argc != 2){
        printf("Correct program usage: ./main probability (must be a float between 0 and 1)\n");
        return 1;
    }

    // Creating shared memory segment between P1 and ENC1
    char *sh_mem = attach_to_block(FIRST_FILE, BLOCK_SIZE, 0);
    if(sh_mem == NULL){
        fprintf(stderr, "Failed to create P1->ENC1 shared memory segment (in P1).\n");
    }
    memset(sh_mem, 0, BLOCK_SIZE);
    
    // Direction = 1 means it is P1 that asked for keyboard input.
    // Direction = 2 means it is P1 that will print a message it will receive on the console.
    int direction = 1;
    // Transmitted = 0 means that it is P1 that asked for keyboard input, but the message is not
    // yet on its way
    // Transmitted = 1 means that it is P1 that asked for keyboard input, and the message is on
    // its way. So, if P1_read is signaled, P1 is expecting the "TRANSMISSION_OK" control message
    int transmitted = 0;
    // If P1 either reads 'bye' from the keyboard, or receives the TERM control message from ENC1
    // (a.k.a. P2 read 'bye' from the keyboard), term becomes 1 and P1 waits for ENC1 and CHAN 
    // to terminate before it also does.
    int term = 0;
    
    // Semaphore ops. P1 will create the named semaphores, ENC1 will simply request them.
    sem_unlink(MUTEX1);
    sem_unlink(P1_READ);
    sem_unlink(P1_WRITE);
    sem_unlink(ENC11_READ);
    sem_unlink(ENC11_WRITE);
    
    sem_t *mutex = sem_open(MUTEX1, O_CREAT, 0600, 1);    
    sem_t *p1r = sem_open(P1_READ, O_CREAT, 0660, 0);
    sem_t *p1w = sem_open(P1_WRITE, O_CREAT, 0660, 1);
    sem_t *enc11r = sem_open(ENC11_READ, O_CREAT, 0660, 0);
    sem_t *enc11w = sem_open(ENC11_WRITE, O_CREAT, 0660, 0);
    
    printf("Messages must not exceed 50 characters.\nWrite 'bye' for termination\n");
    pid_t pid = fork();
    if(pid == 0){
        char *enc1 = "enc/enc1";
        if(execlp(enc1, enc1, argv[1], NULL) == -1){
            perror("error code from execlp: ");
        }
    }else{
        msg *input = malloc(sizeof(msg));
        input->message = malloc(50*sizeof(char));
        
        while(1){
            if(direction == 1 && transmitted == 0){
                sem_wait(p1w);
                sem_wait(mutex);
                
                printf("Input: ");
                fgets(input->message, 50, stdin);
                input->message = strtok(input->message, "\n");
                if(strcmp(input->message, "bye") == 0){
                    strcpy(input->message, "TERM");
                    term = 1;
                }
                input->length = strlen(input->message);
                memcpy(sh_mem, input, sizeof(int));
                memcpy(sh_mem + sizeof(int), input->message, input->length*sizeof(char));
                transmitted = 1;
                
                sem_post(mutex);
                sem_post(enc11r);

            }else if(direction == 1 && transmitted == 1){
                sem_wait(p1r);
                sem_wait(mutex);
                
                memcpy(input, sh_mem, sizeof(int));
                if(memcmp(sh_mem + sizeof(int), "TRANSMISSION_OK", input->length) == 0){
                    // TRANSMISSION_OK is confirmation from P2 that the message P1 read
                    // from keyboard has been printed to the terminal.
                    // P1 confirms it received the confirmation via the "ALL_SET" control
                    // message. In each process, "ALL_SET" will change the value of
                    // direction to indicate which process will expect input from keyboard next.
                    direction = 2;
                    transmitted = 0;
                    sem_post(mutex);
                    sem_post(p1w);
                    
                    // ALL_SET MESSAGE TRANSMISSION
                    sem_wait(p1w);
                    sem_wait(mutex);
                    input->length = strlen("ALL_SET");
                    memcpy(sh_mem, input, sizeof(int));
                    memcpy(sh_mem + sizeof(int), "ALL_SET", input->length);
                    clear_buffer(input);
                    
                    sem_post(mutex);
                    sem_post(enc11r);
                }
                
            }else if(direction == 2){
                sem_wait(p1r);
                sem_wait(mutex);
                
                memcpy(input, sh_mem, sizeof(int));
                if(memcmp(sh_mem + sizeof(int), "ALL_SET", input->length) == 0){
                    // If P1 received ALL_SET, it is its turn to read from stdin again.
                    sem_post(p1w);
                    
                    sem_wait(p1w);
                    clear_buffer(input);
                    direction = 1;
                    printf("Input: ");
                    fgets(input->message, 50, stdin);
                    input->message = strtok(input->message, "\n");
                    if(strcmp(input->message, "bye") == 0){
                        strcpy(input->message, "TERM");
                        term = 1;
                    }
                    input->length = strlen(input->message);
                    memcpy(sh_mem, input, sizeof(int));
                    memcpy(sh_mem + sizeof(int), input->message, input->length*sizeof(char));
                    transmitted = 1;
                    
                    sem_post(mutex);
                    sem_post(enc11r);
                }else if(memcmp(sh_mem + sizeof(int), "TERM", input->length) == 0){
                    term = 1;
                }else{
                    memcpy(input->message, sh_mem + sizeof(int), input->length);
                
                    printf("Received: %s\n", input->message);
                    
                    input->length = strlen("TRANSMISSION_OK");
                    memcpy(sh_mem, input, sizeof(int));
                    memcpy(sh_mem + sizeof(int), "TRANSMISSION_OK", input->length);
                    
                    sem_post(mutex);
                    sem_post(enc11r);
                }
            }
            if(term){
                break;
            }
        }
        
        free(input->message);
        free(input);
       
        wait(NULL);
        if(detach_from_block(sh_mem) == -1){
            fprintf(stderr, "Failed to detach from P1->ENC1 shared memory segment (in P1).\n");
        }
        if(destroy_block(FIRST_FILE, 0) == -1) {
            fprintf(stderr, "Failed to destroy P1->ENC1 shared memory segment (in P1).\n");
        }
        sem_close(mutex);
        sem_close(p1r);
        sem_close(enc11r);
        sem_close(p1w);
        sem_close(enc11w);
    }
}