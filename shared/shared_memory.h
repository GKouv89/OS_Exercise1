#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdio.h>
#include "message_format.h"

#define BLOCK_SIZE 76 // 16 BYTES FOR THE MD5 HASH FUNCTION RESULT, AND WE ASSUME OUR MESSAGE 
// IS NO LONGER THAN 49 CHARACTERS LONG
#define FIRST_FILE "Makefile" // this will be used for shared memory creation/attachment between
// all shared memory segments, but ftok's second argument will change

char *attach_to_block(char*, int, int);
int detatch_from_block(char*);
int destroy_block(char*, int);
void clear_buffer(msg *);

#endif