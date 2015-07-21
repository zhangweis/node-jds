console.log('mock-buffertools');

Buffer.prototype.clear = function(){
    return this.fill(0);
}
Buffer.prototype.fromHex = function(){
    return new Buffer(this.toString('ascii'), 'hex');
}
Buffer.prototype.concat = function(buffer){
    return Buffer.concat([this, buffer]);
}
Buffer.prototype.toHex = function(){
    return this.toString('hex');
}
Buffer.prototype.reverse = function(){
    return new Buffer(this.toByteArray());
}
Buffer.prototype.toByteArray = function () {
  return Array.prototype.slice.call(this, 0)
}

