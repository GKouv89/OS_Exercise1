#ifndef MESSAGE_FORMAT_H
#define MESSAGE_FORMAT_H

#include <openssl/md5.h>

typedef struct format{
    int length;
    char *message;
    char hash[MD5_DIGEST_LENGTH];
} msg;

#endif