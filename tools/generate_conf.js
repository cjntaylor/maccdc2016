'use strict';
var fs = require('fs');
var path = require('path');

var conf = {
  team:  parseInt(process.argv[2]),
  id:    parseInt(process.argv[3]),
  color: [
    parseInt(process.argv[4]),
    parseInt(process.argv[5]), 
    parseInt(process.argv[6])
  ],
  name: process.argv[7]
};
conf = JSON.stringify(conf, null, 2);
console.log(conf);
fs.writeFileSync(path.join(__dirname, '..', 'data', 'conf.json'), conf);