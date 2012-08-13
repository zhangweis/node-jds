#!/usr/bin/env node

var fs = require('fs');

var Bitcoin = require('../lib/bitcoin');

// Get list of blk????.dat files from original client directory
var homeDir = process.env.HOME + "/.bitcoin/";
var homeDirFiles = fs.readdirSync(homeDir);
var blkFileRegex = /^blk[0-9]{4}\.dat$/;
var blkFiles = homeDirFiles.filter(function (f) {
  return blkFileRegex.test(f);
});
// Make paths absolute
blkFiles = blkFiles.map(function (f) {
  return homeDir + f;
});

node = new Bitcoin.Node();
node.cfg.network.connect = "none";
node.cfg.network.noListen = true;
node.cfg.storage.importFrom = blkFiles;
node.start();
