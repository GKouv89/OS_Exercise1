#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "shared_memory.h"
#include "message_format.h"

char *attach_to_block(char* file_name, int size, int arg2){
    // arg2 refers to ftok's second argument
    // for each shared memory segment created by the 5 processes
    // (4 segments in total)
    // ftok will be called with either 0, 1, 2, or 3.
    // If the segment has already been created, a second call to
    // ftok will result in attaching to the already existing segment
    key_t shmem_key = ftok(file_name, arg2);
    
    if(shmem_key == -1){
        return NULL;
    }
    
    // create if shared memory doesn't exist
    // find if it does
    int shmem_id = shmget(shmem_key, size, 0644 | IPC_CREAT);
    if(shmem_id == -1){
        return NULL;
    }
    
    
    char *sh_mem = shmat(shmem_id, NULL, 0);
    if(sh_mem == (char *)-1){
        return NULL;
    }
    
    return sh_mem;
}

int detach_from_block(char* sh_mem){
    return shmdt(sh_mem);
}

int destroy_block(char *file_name, int arg2){
    key_t shmem_key = ftok(file_name, arg2);
    
    if(shmem_key == -1){
        return -1;
    }
    
    // create if shared memory doesn't exist
    // find if it does
    int shmem_id = shmget(shmem_key, 0, 0644 | IPC_CREAT);
    if(shmem_id == -1){
        return -1;
    }

    return shmctl(shmem_id, IPC_RMID, NULL);
}

void clear_buffer(msg *input){
    // when done with transmitting a message and preparing for the next one,
    // this function empties the msg buffer all processes posses
    // without messing with how the memory is allocated
    memset(input->message, 0, 50);
    memset(input + sizeof(int) + sizeof(char*), 0, MD5_DIGEST_LENGTH);
    memset(input, 0, sizeof(int));
}


