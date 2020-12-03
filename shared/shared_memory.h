#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdio.h>
#include <openssl/md5.h>
#include "message_format.h"

// WE ACCEPT THAT OUR MESSAGE IS NO LONGER THAN 50 CHARACTERS LONG. 
// WE ALSO NEED AN INT FOR THE LENGTH OF THE MESSAGE AND SPACE FOR THE CHECKSUM, TOO.
#define BLOCK_SIZE sizeof(int)+50*sizeof(char)+MD5_DIGEST_LENGTH
// this will be used for shared memory creation/attachment between
// all shared memory segments, but ftok's second argument will change
#define FIRST_FILE "Makefile" 

char *attach_to_block(char*, int, int);
int detach_from_block(char*);
int destroy_block(char*, int);
void clear_buffer(msg *);

#endif