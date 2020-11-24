#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "shared_memory.h"

char *attach_to_block(char* file_name, int size, int arg2){
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

int detatch_from_block(char* sh_mem){
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
