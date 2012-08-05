#include "crypt.h"
#include "openssl/sha.h"

void Crypt::HashDoubleSha256(const char *data, size_t len, uint256_t *hash)
{
  SHA256_CTX ctx;

  SHA256_Init(&ctx);
  SHA256_Update(&ctx, data, len);
  SHA256_Final((unsigned char*)hash, &ctx);
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, (unsigned char*)hash, SHA256_DIGEST_LENGTH);
  SHA256_Final((unsigned char*)hash, &ctx);
}
