{
  "target_defaults": {
    "default_configuration": "Release",
    "cflags!": [ "-fno-exceptions" ],
    "cflags_cc!": [ "-fno-exceptions" ],
    "conditions": [
      ["OS==\"mac\"", {
        "xcode_settings": {
          "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
        }
      }],
      ['target_arch=="ia32"', {
        'variables': {'openssl_config_path':
                      '<(nodedir)/deps/openssl/config/piii'},
      }, {
        'variables': {'openssl_config_path':
                      '<(nodedir)/deps/openssl/config/k8'},
      }]
    ],
    "include_dirs": [
      "src/cpp",
      "src/cpp/deps",
      "src/cpp/port/<(OS)",
      "node_modules/leveldb/src/cpp",
      "node_modules/leveldb/deps/leveldb/include",
      "<(nodedir)/deps/openssl/openssl/include",
      "<(openssl_config_path)"
    ]
  }
}
