/**
 * Simple synchronous parser based on node-binary.
 */
var Parser = exports.Parser = function Parser(buffer)
{
  this.subject = buffer;
  this.pos = 0;
};

Parser.prototype.buffer = function buffer(len) {
  var buf = this.subject.slice(this.pos, this.pos+len);
  this.pos += len;
  return buf;
};

Parser.prototype.search = function search(needle) {
  var len;

  if ("string" === typeof needle || Buffer.isBuffer(needle)) {
    // TODO: Slicing is probably too slow
    len = this.subject.slice(this.pos).indexOf(needle);
    if (len !== -1) {
      this.pos += len + needle.length;
    }
    return len;
  }
  if ("number" === typeof needle) {
    needle = needle & 0xff;
    // Search for single byte
    for (var i = this.pos, l = this.subject.length; i < l; i++) {
      if (this.subject[i] == needle) {
        len = i - this.pos;
        this.pos = i+1;
        return len;
      }
    }
    return -1;
  }
};

/**
 * Like search(), but returns the skipped bytes
 */
Parser.prototype.scan = function scan(needle) {
  var startPos = this.pos;
  var len = this.search(needle);
  if (len !== -1) {
    return this.subject.slice(startPos, startPos+len);
  } else {
    throw new Error('No match');
  }
};

Parser.prototype.eof = function eof() {
  return this.pos >= this.subject.length;
};

Parser.prototype.parseVarInt = function ()
{
  var firstByte = this.word8();
  switch (firstByte) {
  case 0xFD:
    return this.word16le();

  case 0xFE:
    return this.word32le();

  case 0xFF:
    return this.word64le();

  default:
    return firstByte;
  }
};

Parser.prototype.parseVarStr = function ()
{
  var len = this.parseVarInt();
  return this.buffer(len);
};

Parser.prototype.parseBlockHeader = function ()
{
  var data = {};
  data.version = this.word32le();
  data.prev_hash = this.buffer(32);
  data.merkle_root = this.buffer(32);
  data.timestamp = this.word32le();
  data.bits = this.word32le();
  data.nonce = this.word32le();

  return data;
};

Parser.prototype.parseTx = function ()
{
  var data = {}, i, sLen, startPos = this.pos;

  data.version = this.word32le();

  var txinCount = this.parseVarInt();

  data.ins = [];
  for (j = 0; j < txinCount; j++) {
    data.ins.push(this.parseTxIn());
  }

  var txoutCount = this.parseVarInt();

  data.outs = [];
  for (j = 0; j < txoutCount; j++) {
    data.outs.push(this.parseTxOut());
  }

  data.lock_time = this.word32le();

  var endPos = this.pos;

  data.buffer = this.subject.slice(startPos, endPos);

  return data;
};

Parser.prototype.parseTxIn = function ()
{
  var data = {};
  data.o = this.buffer(36);               // outpoint
  var sLen = this.parseVarInt();          // script_len
  data.s = this.buffer(sLen);             // script
  data.q = this.word32le();               // sequence
  return data;
};

Parser.prototype.parseTxOut = function ()
{
  var data = {};
  data.v = this.buffer(8);               // value
  var sLen = this.parseVarInt();         // script_len
  data.s = this.buffer(sLen);            // script
  return data;
};

// convert byte strings to unsigned little endian numbers
function decodeLEu (bytes) {
    var acc = 0;
    for (var i = 0; i < bytes.length; i++) {
        acc += Math.pow(256,i) * bytes[i];
    }
    return acc;
}

// convert byte strings to unsigned big endian numbers
function decodeBEu (bytes) {
    var acc = 0;
    for (var i = 0; i < bytes.length; i++) {
        acc += Math.pow(256, bytes.length - i - 1) * bytes[i];
    }
    return acc;
}

// convert byte strings to signed big endian numbers
function decodeBEs (bytes) {
    var val = decodeBEu(bytes);
    if ((bytes[0] & 0x80) == 0x80) {
        val -= Math.pow(256, bytes.length);
    }
    return val;
}

// convert byte strings to signed little endian numbers
function decodeLEs (bytes) {
    var val = decodeLEu(bytes);
    if ((bytes[bytes.length - 1] & 0x80) == 0x80) {
        val -= Math.pow(256, bytes.length);
    }
    return val;
}

function getDecoder(len, fn) {
  return function () {
    var buf = this.buffer(len);
    return fn(buf);
  };
};
[ 1, 2, 4, 8 ].forEach(function (bytes) {
  var bits = bytes * 8;

  Parser.prototype['word' + bits + 'le']
    = Parser.prototype['word' + bits + 'lu']
    = getDecoder(bytes, decodeLEu);

  Parser.prototype['word' + bits + 'ls']
    = getDecoder(bytes, decodeLEs);

  Parser.prototype['word' + bits + 'be']
    = Parser.prototype['word' + bits + 'bu']
    = getDecoder(bytes, decodeBEu);

  Parser.prototype['word' + bits + 'bs']
    = getDecoder(bytes, decodeBEs);

  Parser.prototype.word8 = Parser.prototype.word8u = Parser.prototype.word8be;
  Parser.prototype.word8s = Parser.prototype.word8bs;
});
