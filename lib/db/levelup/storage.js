var logger = require('../../logger');
var Step = require('step');
var Storage = require('../../storage').Storage;
var Connection = require('../../connection').Connection;
var util = require('util');
var fs = require('fs');
var path = require('path');
var url = require('url');
var mkdirp = require('mkdirp');

var existsSync = fs.existsSync || path.existsSync;

var leveldb = require('levelup'); // database

var Block = require('../../schema/block').Block;
var Transaction = require('../../schema/transaction').Transaction;

function serializeBlock(block)
{
  var data = {
    prev_hash: block.prev_hash.toString('binary'),
    merkle_root: block.merkle_root.toString('binary'),
    timestamp: block.timestamp,
    bits: block.bits,
    nonce: block.nonce,
    version: block.version,
    height: block.height,
    size: block.size,
    active: block.active,
    chainWork: block.chainWork.toString('binary'),
    txs: block.txs.map(function (hash) {
      return hash.toString('binary');
    })
  };

  return JSON.stringify(data);
};

function deserializeBlock(data) {
  data = JSON.parse(data);
  data.prev_hash = new Buffer(data.prev_hash, 'binary');
  data.merkle_root = new Buffer(data.merkle_root, 'binary');
  data.chainWork = new Buffer(data.chainWork, 'binary');
  data.txs = data.txs.map(function (hash) {
      return new Buffer(hash, 'binary');
  });
  return new Block(data);
};

function serializeTransaction(tx) {
  return tx.getBuffer();
};

function deserializeTransaction(data) {
  return new Transaction(Connection.parseTx(data));
};

function formatHeightKey(height) {
  var tempHeightBuffer = new Buffer(4);
  height = Math.floor(+height);
  tempHeightBuffer[0] = height >> 24 & 0xff;
  tempHeightBuffer[1] = height >> 16 & 0xff;
  tempHeightBuffer[2] = height >>  8 & 0xff;
  tempHeightBuffer[3] = height       & 0xff;
  return tempHeightBuffer;
};

function parseHeightKey(height) {
  return (height[0] << 24) +
         (height[1] << 16) +
         (height[2] <<  8) +
         (height[3]      );
};

var LeveldbStorage = exports.LeveldbStorage = exports.Storage =
function LeveldbStorage(uri) {
  var self = this;

  var hMain;
  var bBlockTxsIndex;
  var bTxAffectsIndex;

  // Database version
  var MAJOR_VERSION = 1;
  var MINOR_VERSION = 0;

  var connInfo = url.parse(uri);
  var prefix = connInfo.path.trim();

  var defaultCreateOpts = {
    create_if_missing: true,
    max_open_files: 50,
    write_buffer_size: 100 * 1024 * 1024

   , encoding : "binary"
  };

  var defaultGetOpts = {
    as_buffer: true
   , encoding : "binary"
  };

  var connected = false;
  var metadata;
  var currentBatch = null;

  var connect = this.connect = function connect(callback) {
    if (connected) {
      callback(null);
      return;
    }
    connected = true;

    logger.info("Initializing LevelDB ("+uri+")");

    var baseDir = prefix.substr(-1) == "/" ? prefix : path.dirname(prefix);
    try {
      if (!existsSync(baseDir)) {
        mkdirp.sync(baseDir, 0755);
      }
    } catch (err) {
      logger.error("Could not create datadir '"+baseDir+"': " +
                   (err.stack ? err.stack : err));
      callback(new Error("Unable to access database folder"));
      return;
    }

    var isNew = false;
    Step(
      function createMainDb() {
        leveldb(prefix+'main.db', defaultCreateOpts, this);
      },
      function initMetadata(err, db) {
        if (err) throw err;

        self.hMain = hMain = db;

        var callback = this;

        var keys = 'majorVersion,minorVersion,chainHeight,isRunning'.split(',');
        getMeta(keys, function (err, data) {
          try {
            if (!err) {
              isNew = false;
              metadata = data;
              callback();
            } else if (err.name == "NotFoundError"){
              // New database, create fresh metadata
              isNew = true;
              metadata = {
                majorVersion: MAJOR_VERSION,
                minorVersion: MINOR_VERSION,
                chainHeight: 0,
		isRunning: 0
              };
              setMeta(metadata, callback);
            } else {
              throw err;
            }
          } catch (e) {
            callback(e);
          }
        });
      },
                function checkState(err) {
                  if (err) throw err;
                  if (metadata.isRunning !== 0) {
                    metadata.isRunning = 1;
                    metadata.chainHeight -= 10;
                    if (metadata.chainHeight < 0) metadata.chainHeight = 0;
                    logger.error('Database was not shut down properly! Setting chain height back (to '+metadata.chainHeight+') to re-download last blocks...');
                    setMeta({
                      chainHeight: metadata.chainHeight,
                      isRunning: 1
                    }, this);
                  }
                  
                  metadata.isRunning = 1;
                  setMeta('isRunning', 1, this);
                },

      function createBlockTxsIndexDb(err) {
        if (err) throw err;

        leveldb(prefix+'blocktx.db', defaultCreateOpts, this);
      },
      function createTxAffectsIndexDb(err, db) {
        if (err) throw err;

        self.bBlockTxsIndex = bBlockTxsIndex = db;

        leveldb(prefix+'affects.db', defaultCreateOpts, this);
      },
      function postStep(err, db) {
        if (err) throw err;

        self.bTxAffectsIndex = bTxAffectsIndex = db;

        logger.info("LevelDB: "+
                    (isNew ? "New database created" : "Database loaded") +
                    " (rev. " +
                    metadata.majorVersion + "." +
                    metadata.minorVersion + ")");

        this();
      },
      callback
    );
  };

  var disconnect = this.disconnect = function disconnect(callback) {
    if (!connected) {
      callback(null);
      return;
    }
            Step(
              function updateState() {
                setMeta('isRunning', 0, this);
              },
              function disconnectDbs(err) {
                if (err) throw err;
                var group = this.group();
                
                hMain.close(group());
                bBlockTxsIndex.close(group());
                bTxAffectsIndex.close(group());
              },
              function cleanup(err) {
                if (err) throw err;
                delete hMain;
                delete bBlockTxsIndex;
                delete bTxAffectsIndex;
                return null;
              },
              callback
            );
/*
    delete hMain;
    delete bBlockTxsIndex;
    delete bTxAffectsIndex;

    callback();
*/
  };

  var emptyDatabase = this.emptyDatabase =
  function emptyDatabase(callback) {
    Step(
      function () {
        disconnect(this);
      },
      function (err) {
        if (err) throw err;

        leveldb.destroy(prefix+'main.db', this);
      }, function (err) {
        if (err) throw err;

        leveldb.destroy(prefix+'blocktx.db', this);
      }, function (err) {
        if (err) throw err;

        leveldb.destroy(prefix+'affects.db', this);
      },
      function (err) {
        if (err) throw err;
        connected = false;
        connect(this);
      },
      callback
    );
  };

  this.dropDatabase = function (callback) {
    Step(
      function () {
        leveldb.destroy(prefix+'main.db', this);
      }, function (err) {
        if (err) throw err;

        leveldb.destroy(prefix+'blocktx.db', this);
      }, function (err) {
        if (err) throw err;

        leveldb.destroy(prefix+'affects.db', this);
      }, callback);
  };

  var setMeta = this.setMeta = function (key, value, callback) {
    if ("string" === typeof key) {
      metadata[key] = value;
      hMain.put(key, JSON.stringify(value), callback);
    } else if ("object" === typeof key) {
      var keys = key;
      callback = value;

      var steps = [];
      Object.keys(keys).forEach(function (key) {
        var value = keys[key];

        metadata[key] = value;
        steps.push(function (err) {
          if (err) throw err;

          hMain.put(key, JSON.stringify(value), this);
        });
      });

      if ("function" === typeof callback) {
        steps.push(callback);
      }

      Step.apply(Step, steps);
    } else {
      throw new Error('Invalid key type for setMeta: ' + typeof key);
    }
  };

  var getMeta = this.getMeta = function (key, callback) {
    if ("string" === typeof key) {
      hMain.get(key, function (err, val) {
        if (err) {
          callback(err);
          return;
        }

        if ("string" === typeof val) {
          callback(null, JSON.parse(val));
        } else {
          callback(null, undefined);
        }
      });
    } else if (Array.isArray(key)) {
      var map = {};
      var steps = key.map(function (key) {
        return function (err) {
          if (err) throw err;

          var callback = this;
          hMain.get(key, function (err, val) {
            if (err) {
              callback(err);
            } else {
              if ("string" === typeof val) {
                map[key] = JSON.parse(val);
              }
              callback();
            }
          });
        };
      });
      steps.push(function (err) {
        if (err) throw err;

        this(null, map);
      });

      if ("function" === typeof callback) {
        steps.push(callback);
      }

      Step.apply(Step, steps);
    } else {
      throw new Error('Invalid key type for getMeta: ' + typeof key);
    }
  };

  var startTransaction = this.startTransaction = function (callback) {
    // TODO: Currently, transactions are only used by the BlockChain. Once we
    // want the ability to have multiple parallel transactions, we need a better
    // way to pass the batch object to the 
    try {
      currentBatch = []
      if ("function" === typeof callback) {
        callback(null);
      }
    } catch (e) {
      if ("function" === typeof callback) {
        callback(e);
      }
    }
  };

  var endTransaction = this.endTransaction = function (callback) {
    if (currentBatch !== null) {
      hMain.batch(currentBatch, callback);
      currentBatch = null;
    } else {
      if ("function" === typeof callback) {
        callback(null);
      }
    }
  };

  this.saveBlock = function (block, callback) {
    var hash = block.getHash();
    var data = serializeBlock(block);
    Step(
      function () {
        hMain.put(hash, data, this);
      },
      function (err) {
        if (err) throw err;

        // TODO: Encode as integer
        var height = formatHeightKey(block.height);
        hMain.put(height, hash, this);
      },
      function (err) {
        if (err) throw err;

        if (block.active && metadata.chainHeight < block.height) {
          setMeta('chainHeight', block.height);
        }

        var wb = [];
        block.txs.forEach(function (txHash) {
          wb.push({type: 'put', key: txHash, value:hash});
        });
        bBlockTxsIndex.batch(wb, this);
      },
      callback
    );
  };

  this.saveTransaction = function (tx, callback) {
    var hash = tx.getHash();
    var data = serializeTransaction(tx);
    hMain.put(hash, data, callback);
  };

  this.saveTransactions = function (txs, callback) {
    var wb = currentBatch ? currentBatch : [];
    txs.forEach(function (tx) {
      wb.push({type: 'put', key:tx.getHash(), value:serializeTransaction(tx)});
    });
    if (currentBatch !== null) hMain.batch(wb, callback);
    else callback(null);
  };

  var connectTransaction = this.connectTransaction =
  function connectTransaction(tx, callback) {
    connectTransactions([tx], callback);
  };

  var connectTransactions = this.connectTransactions =
  function connectTransactions(txs, callback) {
    Step(
      function saveSpent() {
        var wb = currentBatch ? currentBatch : [];
        txs.forEach(function (tx) {
          if (tx.isCoinBase()) {
            return;
          }
          var hash = tx.getHash();
          tx.ins.forEach(function (txin) {
            wb.push({ type: "put", key: txin.o, value: hash });
          });
        });
        if (currentBatch !== null) hMain.batch(wb, callback);
        else this(null);
      },
      function saveAffects(err) {
        if (err) throw err;

        var ops = [];
        txs.forEach(function (tx) {
          if (tx.affects) {
            tx.affects.forEach(function (affect) {
              var key = affect.concat(tx.getHash());
              ops.push({ type: 'put', key: key });
            });
          }
        });
        bTxAffectsIndex.batch(ops);
      },
      callback
    );
  };

  var disconnectTransaction = this.disconnectTransaction =
  function disconnectTransaction(tx, callback) {
    disconnectTransactions([tx], callback);
  };

  var disconnectTransactions = this.disconnectTransactions =
  function disconnectTransactions(txs, callback) {
    var wb = currentBatch ? currentBatch : [];
    txs.forEach(function (tx) {
      tx.ins.forEach(function (txin) {
        wb.push({type: "del", key:txin.o});
      });
    });
    if (currentBatch !== null) hMain.batch(wb, callback);
    else callback(null);
  };

  var getTransactionByHash = this.getTransactionByHash =
  function getTransactionByHash(hash, callback) {
    hMain.get(hash, defaultGetOpts, function (err, data) {
      if (err) {
        callback(err);
        return;
      }
      if (data) {
        data = deserializeTransaction(data);
      }
      callback(null, data);
    });
  };

  var getTransactionsByHashes = this.getTransactionsByHashes =
  function getTransactionsByHashes(hashes, callback) {
    Step(
      function () {
        var group = this.group();
        for (var i = 0, l = hashes.length; i < l; i++) {
          hMain.get(hashes[i], defaultGetOpts, group());
        }
      },
      function (err, result) {
        if (err) throw err;
        var txs = [];
        result.forEach(function (tx) {
          if (tx) {
            txs.push(deserializeTransaction(tx));
          }
        });
        this(null, txs);
      },
      callback
    );
  };

  this.getOutputsByHashes = function (hashes, callback) {
    getTransactionsByHashes(hashes, callback);
  };

  var getBlockByHash = this.getBlockByHash =
  function getBlockByHash(hash, callback) {
    hMain.get(hash, defaultGetOpts, function getBlockByHashCallback(err, data) {
      if (err && err.name != 'NotFoundError') {
        callback(err);
        return;
      }

      if (data) {
        data = deserializeBlock(data);
      }

      callback(null, data);
    });
  };

  var getBlocksByHashes = this.getBlocksByHashes =
  function getBlocksByHashes(hashes, callback) {
    Step(
      function () {
        var group = this.group();
        for (var i = 0, l = hashes.length; i < l; i++) {
          if (hashes[i]) {
            hMain.get(hashes[i], defaultGetOpts, group());
          }
        }
      },
      function (err, result) {
        if (err) throw err;

        var blocks = [];
        result.forEach(function (block) {
          if (block) {
            blocks.push(deserializeBlock(block));
          }
        });

        callback(null, blocks);
      },
      callback
    );
  };

  var getBlockByHeight = this.getBlockByHeight =
  function getBlockByHeight(height, callback) {
    height = formatHeightKey(height);
    Step(
      function () {
        hMain.get(height, defaultGetOpts, this);
      },
      function (err, result) {
        if (err && err.name != "NotFoundError") throw err;

        if (!result) {
          this(null, null);
        } else {
          getBlockByHash(result, this);
        }
      },
      callback
    );
  };

  var getBlocksByHeights = this.getBlocksByHeights =
  function getBlocksByHeights(heights, callback)
  {
    Step(
      function () {
        var group = this.group();
        heights.forEach(function (i) {
          hMain.get(formatHeightKey(i), defaultGetOpts, group());
        });
      },
      function (err, hashes) {
        if (err) throw err;

        getBlocksByHashes(hashes, this);
      },
      function sortStep(err, blocks) {
        if (err) throw err;

        blocks = blocks.sort(function (a, b) {
          return a.height - b.height;
        });

        try {
          callback(null, blocks);
        } catch (err) {
          logger.error('Storage: Uncaught callback err: ' +
                       (err.stack ? err.stack : err.toString()));
        }
      }
    );
  };

  var getBlockByPrev = this.getBlockByPrev =
  function getBlockByPrev(block, callback) {
    if ("object" == typeof block && block.hash) {
      block = block.hash;
    }

    Step(
      function getSelectedBlockStep() {
        getBlockByHash(block, this);
      },
      function getTargetBlockStep(err, block) {
        if (err) throw err;

        if (!block) {
          callback(new Error("Block chain empty or corrupted"));
          return;
        }

        getBlockByHeight(block.height+1, this);
      },
      callback
    );
  };

  var getTopBlock = this.getTopBlock =
  function getTopBlock(callback) {
    getBlockByHeight(metadata.chainHeight, callback);
  };

  var getBlockSlice = this.getBlockSlice =
  function getBlockSlice(start, limit, callback) {
    var steps = [];

    if (start < 0) {
      start = metadata.chainHeight + start;
    }

    var end;
    if (limit < 0) {
      end = metadata.chainHeight + limit;
    } else if (limit == 0) {
      callback(null, []);
    } else if ("undefined" === typeof limit) {
      end = metadata.chainHeight;
    } else {
      end = start + limit;
    }

    if (end > metadata.chainHeight) {
      end = metadata.chainHeight;
    } else if (start >= end) {
      callback(null, []);
    }

    var heights = [];
    for (var i = start; i < end; i++) {
      heights.push(i);
    }

    getBlocksByHeights(heights, callback);
  };

  /**
   * Find the latest matching block from a locator.
   *
   * A locator is basically just a list of hashes. We send it to the database
   * and ask it to get the latest block that is in the list.
   */
  var getBlockByLocator = this.getBlockByLocator =
  function (locator, callback)
  {
    getBlocksByHashes(locator, function (err, blocks) {
      if (err) {
        callback(err);
        return;
      }

      var highest = null;
      blocks.forEach(function (block) {
        if (block.active &&
            ((!highest) || block.height > highest.height)) {
          highest = block;
        }
      });

      callback(null, highest);
    });
  };

  var countConflictingTransactions = this.countConflictingTransactions =
  function countConflictingTransactions(outpoints, callback) {
    Step(
      function queryOutpointsStep() {
        var group = this.group();
        for (var i = 0, l = outpoints.length; i < l; i++) {
          hMain.get(outpoints[i], defaultGetOpts, group());
        }
      },
      function reduceResultStep(err, results) {
        if (err) throw err;

        var count = results.reduce(function(sum, result){  
          return "string" === typeof result ? ++sum : sum;
        }, 0);
        this(null, count);
      },
      callback
    );
  };

  var getConflictingTransactions = this.getConflictingTransactions =
  function getConflictingTransactions(outpoints, callback) {
    Step(
      function queryOutpointsStep() {
        var group = this.group();
        for (var i = 0, l = outpoints.length; i < l; i++) {
          hMain.get(outpoints[i], defaultGetOpts, group());
        }
      },
      function reduceResultStep(err, results) {
        if (err) throw err;

        results = results.filter(function(result){
          return Buffer.isBuffer(result);
        }, 0);

        this(null, results);
      },
      function getTransactionsStep(err, hashes) {
        if (err) throw err;

        getTransactionsByHashes(hashes, this);
      },
      function reduceResultAgainStep(err, results) {
        if (err) throw err;

        results = results.filter(function(result){
          return result instanceof Transaction;
        }, 0);

        this(null, results);
      },
      callback
    );
  };

  var knowsBlock = this.knowsBlock =
  function knowsBlock(hash, callback) {
    getBlockByHash(hash, function (err, block) {
      if (err && err.name != 'NotFoundError') {
        callback(err);
        return;
      }

      callback(null, !!block);
    });
  };

  var knowsTransaction = this.knowsTransaction =
  function knowsTransction(hash, callback) {
    getTransactionByHash(hash, function (err, tx) {
      if (err && err.name != 'NotFoundError') {
        callback(err);
        return;
      }

      callback(null, !!tx);
    });
  };

  var getContainingBlock = this.getContainingBlock =
  function getContainingBlock(txHash, callback)
  {
    Step(
      function queryBlockTxsIndexStep() {
        bBlockTxsIndex.get(txHash, defaultGetOpts, this);
      },
      callback
    );
  };

  var getAffectedTransactions = this.getAffectedTransactions =
  function getAffectedTransactions(addrHashes, callback)
  {
    if (Buffer.isBuffer(addrHashes)) {
      addrHashes = [addrHashes];
    } else if (Array.isArray(addrHashes)) {
      // No change needed
    } else {
      throw new Error('Invalid addrHashes parameter, expected '+
                      'Buffer or array of buffers.');
    }
    var iterator;
    var steps = [];

    steps.push(function () {
      bTxAffectsIndex.iterator({}, this);
    });
    steps.push(function (err, iter) {
      if (err) throw err;

      iterator = iter;

      this();
    });

    // Get affected transactions for each supplied address
    var hashes = []; // hack, see below
    addrHashes.forEach(function (addrHash) {
      steps.push(function rewindIteratorStep(err) {
        if (err) throw err;

        iterator.seek(addrHash, this);
      });
      steps.push(function iterateStep(err) {
        if (err) throw err;

        var endLoop = this;
        function getNextKey(err) {
          if (err) {
            endLoop(err);
            return;
          }
          iterator.key(defaultGetOpts, function (err, key) {
            if (Buffer.isBuffer(key) && key.slice(0, 20).equals(addrHash)) {
              var buf = new Buffer(key.length - 20);
              key.copy(buf, 0, 20);
              hashes.push(buf);
              iterator.next(getNextKey);
            } else {
              endLoop(null, hashes); // the second argument is for final callback
            }
          });
        };
        getNextKey();
      });
    });
    steps.push(callback);

    Step.apply(null, steps);
  };
};

util.inherits(LeveldbStorage, Storage);
