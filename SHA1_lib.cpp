#include "SHA1_lib.h"
#include <cstddef>

#if OPEN_SSL

#pragma comment (lib, "libcryptoMT.lib")

#endif

#if CRYPTO_API

int SHA1_Init(SHA_CTX* ctx)
{
    DWORD dwStatus = 0;
    // Get handle to the crypto provider
    if (!CryptAcquireContext(&ctx->hProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        return dwStatus;
    }

    if (!CryptCreateHash(ctx->hProv, CALG_SHA1, 0, 0, &ctx->hHash))
    {
        dwStatus = GetLastError();
        CryptReleaseContext(ctx->hProv, 0);
        return dwStatus;
    }

    return dwStatus;
}

int SHA1_Update(SHA_CTX* ctx, const void* data, size_t len)
{
    DWORD dwStatus = 0;

    if (!CryptHashData(ctx->hHash, (const BYTE*)data, (DWORD)len, 0))
    {
        dwStatus = GetLastError();
        CryptReleaseContext(ctx->hProv, 0);
        CryptDestroyHash(ctx->hHash);
    }
    return dwStatus;
}

int SHA1_Final(unsigned char* md, SHA_CTX* ctx)
{
    DWORD dwStatus = 0;
    DWORD cbHash = SHA_DIGEST_LENGTH;
    if (!CryptGetHashParam(ctx->hHash, HP_HASHVAL, md, &cbHash, 0))
    {
        dwStatus = GetLastError();
    }

    return dwStatus;
}

void SHA1_Clean(SHA_CTX* ctx)
{
    CryptReleaseContext(ctx->hProv, 0);
    CryptDestroyHash(ctx->hHash);
}

unsigned char* SHA1(const unsigned char* d, size_t n, unsigned char* md)
{
    SHA_CTX ctx;
    static unsigned char m[SHA_DIGEST_LENGTH];

    if (md == NULL)
        md = m;

    SHA1_Init(&ctx);
    SHA1_Update(&ctx, d, n);
    SHA1_Final(md, &ctx);
    SHA1_Clean(&ctx);
    return md;
}
#endif

#if !CRYPTO_API && !OPEN_SSL

#include "sha1.cpp"

int SHA1_Init(SHA_CTX* ctx)
{
    SHA1Reset(ctx);

    return 0;
}

int SHA1_Update(SHA_CTX* ctx, const void* data, size_t len)
{
    SHA1Input(ctx, (const unsigned char*)data, (unsigned int)len);

    return 0;
}

int SHA1_Final(unsigned char* md, SHA_CTX* ctx)
{
    int Status = SHA1Result(ctx);

    for (int i = 0; i < 5; i++) {
        *md++ = ctx->Message_Digest[i] >> 24;
        *md++ = ctx->Message_Digest[i] >> 16;
        *md++ = ctx->Message_Digest[i] >> 8;
        *md++ = ctx->Message_Digest[i] >> 0;
    }

    return Status;
}


unsigned char* SHA1(const unsigned char* d, size_t n, unsigned char* md)
{
    SHA_CTX ctx;
    static unsigned char m[SHA_DIGEST_LENGTH];

    if (md == NULL)
        md = m;

    SHA1_Init(&ctx);
    SHA1_Update(&ctx, d, n);
    SHA1_Final(md, &ctx);
    return md;
}

#endif