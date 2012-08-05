{
  "targets": [
    {
      "target_name": "bitcoinjs",
      "sources": [
        "src/cpp/main.cc",
        "src/cpp/common.h",
        "src/cpp/eckey.cc",
        "src/cpp/eckey.h",
        "src/cpp/leveldb.cc",
        "src/cpp/leveldb.h",
        "src/cpp/util/crypt.cc",
        "src/cpp/util/crypt.h",
        "src/cpp/util/hex.cc",
        "src/cpp/util/hex.h",
        "src/cpp/util/inttypes.h",
        "src/cpp/util/parser.cc",
        "src/cpp/util/parser.h"
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
