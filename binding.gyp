{
  "targets": [
    {
      "target_name": "bitcoinjs",
      "sources": [
        "src/main.cc",
        "src/common.h",
        "src/eckey.cc",
        "src/eckey.h",
        "src/leveldb.cc",
        "src/leveldb.h",
        "src/util/crypt.cc",
        "src/util/crypt.h",
        "src/util/hex.cc",
        "src/util/hex.h",
        "src/util/inttypes.h",
        "src/util/parser.cc",
        "src/util/parser.h"
      ],
      "link_settings": {
        "libraries": [
          "../node_modules/leveldb/build/Release/leveldb.node"
        ],
        "ldflags": [
          "-Wl,-rpath='$$ORIGIN/../../node_modules/leveldb/build/Release/'"
        ]
      }
    }
  ]
}
