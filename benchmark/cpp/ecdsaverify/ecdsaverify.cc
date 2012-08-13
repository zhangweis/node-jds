#include <openssl/rand.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <hayai.hpp>

#include "util/inttypes.h"

using namespace bitcoinjs;

#define ECDSA_ITERATIONS 50
#define ECDSA_SIG_SIZE 72
#define ECDSA_POINT_SIZE 65

class ECDSAVerifyFixture
: public Hayai::Fixture
{
  virtual void SetUp()
  {
    uint32_t i;

    // Generate random signature hashes
    RAND_pseudo_bytes(reinterpret_cast<uint8_t*>(hashes),
                      sizeof(uint256_t)*ECDSA_ITERATIONS);

    for (i = 0; i < ECDSA_ITERATIONS; i++) {
      EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);

      if (!EC_KEY_generate_key(key)) {
        assert(false);
      }

      uint32_t pub_size = i2o_ECPublicKey(key, NULL);
      assert(pub_size == ECDSA_POINT_SIZE);

      uint8_t *pub_pointer = points[i];
      pub_size = i2o_ECPublicKey(key, &pub_pointer);
      assert(pub_size == ECDSA_POINT_SIZE);

      ECDSA_SIG *sig = ECDSA_do_sign(hashes[i].d8, 32, key);

      uint32_t der_size = i2d_ECDSA_SIG(sig, NULL);
      assert(der_size <= ECDSA_SIG_SIZE);
      sigSizes[i] = der_size;

      uint8_t *der_pointer = sigs[i];
      der_size = i2d_ECDSA_SIG(sig, &der_pointer);
      assert(der_size <= ECDSA_SIG_SIZE);

      ECDSA_SIG_free(sig);
    }

    iteration = 0;
  }

  virtual void TearDown()
  {
  }

public:
  const EC_GROUP *group;
  uint256_t hashes[ECDSA_ITERATIONS];
  uint8_t points[ECDSA_POINT_SIZE][ECDSA_ITERATIONS];
  uint8_t sigs[ECDSA_SIG_SIZE][ECDSA_ITERATIONS];
  uint8_t sigSizes[ECDSA_ITERATIONS];
  uint32_t iteration;
};

BENCHMARK_F(ECDSAVerifyFixture, ECDSAVerify_OpenSSL, 20, ECDSA_ITERATIONS)
{
  EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);

  // Deserialize public key
  const uint8_t *pub_pointer = points[iteration];
  o2i_ECPublicKey(&key, &pub_pointer, ECDSA_POINT_SIZE);

  // Check signature
  bool result = ECDSA_verify(0,
                             hashes[iteration].d8, 32,
                             sigs[iteration], sigSizes[iteration],
                             key);

  //assert(result);

  ++iteration;
}
