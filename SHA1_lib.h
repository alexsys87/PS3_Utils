#pragma once

//#define CRYPTO_API 1
#define OPEN_SSL 1

#if CRYPTO_API
#include <windows.h>
#include <Wincrypt.h>
typedef struct SHAstate_st {
    HCRYPTPROV hProv;
    HCRYPTHASH hHash;
} SHA_CTX;
#endif
#if OPEN_SSL
#include <openssl/sha.h>
#endif
#if !CRYPTO_API && !OPEN_SSL
#include "sha1.h"
#define SHA_CTX  SHA1Context
#endif

#define SHA_DIGEST_LENGTH 20

int SHA1_Init(SHA_CTX* ctx);
int SHA1_Update(SHA_CTX* ctx, const void* data, size_t len);
int SHA1_Final(unsigned char* md, SHA_CTX* ctx);
unsigned char* SHA1(const unsigned char* d, size_t n, unsigned char* md);