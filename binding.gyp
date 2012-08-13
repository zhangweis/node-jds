{
  'includes': [
    'common.gypi',
  ],
  "targets": [
    {
      "target_name": "bitcoinjs",
      "sources": [
        "src/cpp/main.cc",
        "src/cpp/common.h",
        "src/cpp/coinsdb.cc",
        "src/cpp/coinsdb.h",
        "src/cpp/database.cc",
        "src/cpp/database.h",
        "src/cpp/eckey.cc",
        "src/cpp/eckey.h",
        "src/cpp/util/crypt.cc",
        "src/cpp/util/crypt.h",
        "src/cpp/util/hex.cc",
        "src/cpp/util/hex.h",
        "src/cpp/util/inttypes.h",
        "src/cpp/util/parser.h",
        "src/cpp/deps/murmur/MurmurHash3.cpp",
        "src/cpp/deps/murmur/MurmurHash3.h"
      ],
      "link_settings": {
        "libraries": [
          "../node_modules/leveldb/build/Release/leveldb.node"
        ],
        "ldflags": [
          "-Wl,-rpath='$$ORIGIN/../../node_modules/leveldb/build/Release/'"
        ]
      }
    },
    {
      "target_name": "benchmark",
      "type": "none",
      "dependencies": [
        "bitcoinjs",
        "benchmark/cpp/benchmark.gyp:*"
      ]
    }
  ]
}
