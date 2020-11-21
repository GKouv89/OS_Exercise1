#ifndef COMMON_KEYS1_H
#define COMMON_KEYS1_H

#define MESSAGE_SIZE 50

key_t p1enc1key = 1234; // This is the key for the shared memory segment shared between p1 and enc1
int mutex_semid;
int empty_semid;
int full_semid;

#endif