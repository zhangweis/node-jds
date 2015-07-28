var bs58 = require('bs58');
var cryptoHashing = require('crypto-hashing');
//var cs = require('coinstring');
//var Address = require('btc-address');
var ecdsa = require('ecdsa');
var BigInteger = require('bigi')

module.exports = {
    base58_encode: bs58.encode,
    base58_decode: function(input) {
        return new Buffer(bs58.decode(input));
    }, 
    pubkey_to_address256: function(pkBuffer) {
        var hash160 = cryptoHashing.ripemd160(cryptoHashing.sha256(pkBuffer));
        var version = 0x00;
        // Get a copy of the hash
        var hash = hash160.slice(0);
      
        // Version
        hash=Buffer.concat([new Buffer([version]), hash]);
      
        var checksum = cryptoHashing.sha256.x2(hash, { in : 'bytes',
          out: 'buffer'
        });
      
        var bytes = Buffer.concat([hash,checksum.slice(0, 4)]);
      
        return bytes;
    },
	BitcoinKey:function() {
		this.verifySignature=function(hash, sig, cb) {
			cb(null, this.verifySignatureSync(hash, sig));
		};
		this.verifySignatureSync= function(hash, sig) {
            var signature = ecdsa.parseSig(sig);
            function ensurePostive(bigNum) {
                if (bigNum.signum()>0) {
                    return bigNum;
                }
                return BigInteger.fromDERInteger(Buffer.concat([new Buffer([0x00]),bigNum.toBuffer()]));
            }
            signature.r = ensurePostive(signature.r);
            signature.s = ensurePostive(signature.s);
			return ecdsa.verify(hash, signature, this.public);
		};
	},
/*
    verifySignature: function(hash, sig, cb) {
        
    },
    verifySignatureSync: function(hash, sig) {
        return true;
    },
    regenerateSync: function() {
        
    },
    toDER: function(){
        return new Buffer();
    },
    signSync: function(hash){
        return new Buffer();
    }, 
    generateSync: function(){
        
    },
    fromDER: function(){
        
    }
*/
}
