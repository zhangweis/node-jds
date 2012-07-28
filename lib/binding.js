var logger = require('./logger');

// Native extensions
try {
  module.exports = require('../build/Release/bitcoinjs.node');
} catch (e) {
  var bpath = require('path').resolve(__dirname, '..');
  logger.error((e.stack ? e.stack : e.toString()) + "\n");
  logger.error("Unable to load native module, try recompiling it:");
  logger.error("  cd "+bpath+"; npm run-script install");
  process.exit(1);
}
