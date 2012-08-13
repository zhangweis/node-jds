{
  'includes': [
    '../../common.gypi',
  ],
  "targets": [
    {
      "target_name": "coinsdb_benchmark",
      "sources": [
        "deps/hayai/hayai-nodejsmain.cc",
        "coinsdb/coinsdb.cc"
      ]
    },
    {
      "target_name": "ecdsaverify_benchmark",
      "sources": [
        "deps/hayai/hayai-nodejsmain.cc",
        "ecdsaverify/ecdsaverify.cc"
      ]
    }
  ],
  "target_defaults": {
      "include_dirs": [
        "deps/hayai/"
      ],
      "link_settings": {
        "libraries": [
          "Debug/bitcoinjs.node"
        ]
      }
  }
}
