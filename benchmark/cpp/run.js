#!/usr/bin/env node

require(__dirname+'/../../lib/binding');
require(__dirname+'/../../build/Debug/'+process.argv[2]+'_benchmark.node');
