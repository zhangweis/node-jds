#include <openssl/rand.h>
#include <hayai.hpp>

#include "coinsdb.h"

using namespace bitcoinjs;

class CoinsDBFixture
: public Hayai::Fixture
{
  virtual void SetUp()
  {
  }

  virtual void TearDown()
  {

  }

public:
  CoinsDB cdb;
};

BENCHMARK_F(CoinsDBFixture, CoinsDB_Insert, 20, 500)
{
  Coins c;
  TxOut o;
  uint256_t hash;

  RAND_pseudo_bytes(hash.d8, 32);
  RAND_pseudo_bytes(reinterpret_cast<unsigned char *>(&o.value), sizeof(o.value));
  c.Set(0, o);
  cdb.Save(hash, c);
}
