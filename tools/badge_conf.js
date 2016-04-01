'use strict';
var dgram  = require('dgram');
var config = require('./config');

if(process.argv.length < 4) {
  console.log('Usage: node badge_conf.js <ADDRESS> <CMD> [ARGS]');
  process.exit(1);
}

function exitError () {
  console.error('Invalid command');
  process.exit(2);
}

function clamp (v, a, b) {
  return (v < a) ? a : ((v > b) ? b : v);
}

function sendPayload (payload, address) {
  var socket = dgram.createSocket('udp4');
  socket.bind(config.address.interface, function () {
    socket.send(payload, 0, payload.length, config.port, address, function (err) {
      if(err) console.error(err);
      setTimeout(function () { socket.close(); }, 500);
    });
  });
}

function updateTeam (address, team, id) {
  console.log('Updating ' + address + ' to team ' + team + '-' + id);
  var payload = new Buffer([
      config.commands.team,
      clamp(team, 1, config.max.team),
      clamp(id  , 1, config.max.id  )
  ]);
  return sendPayload(payload, address);
}

function updateColor (address, r, g, b) {
  console.log('Updating ' + address + ' color to [' + r + ', ' + g + ', ' + b + ']');
  var payload = new Buffer([
    config.commands.color,
    clamp(r, 0, 255),
    clamp(g, 0, 255),
    clamp(b, 0, 255)
  ]);
  return sendPayload(payload, address);
}

function updateTeamColor(team, r, g, b) {
  console.log('Updating Team ' + team + ' color to [' + r + ', ' + g + ', ' + b + ']');
  var payload = new Buffer([
    config.commands.teamcolor,
    team,
    clamp(r, 0, 255), 
    clamp(g, 0, 255), 
    clamp(b, 0, 255)
  ]);
  var socket = dgram.createSocket('udp4');
  socket.bind(config.address.interface, function () {
    socket.send(payload, 0, payload.length, config.port, config.address.multicast, function (err) {
      if(err) console.error(err);
      socket.close();
    });
  });
}

function updateName (address, name) {
  console.log('Updating ' + address + ' name to "' + name + '"');
  var payload = Buffer.concat([new Buffer([config.commands.name]), new Buffer(name)]);
  return sendPayload(payload, address);
}

function sendEcho (address) {
  console.log('Sending ' + address + ' echo command');
  var payload = new Buffer([config.commands.echo]);
  return sendPayload(payload, address);
}

var cmd = parseInt(process.argv[2]);
switch(cmd) {
  case 1:
    if(process.argv.length < 6) exitError();
    updateTeam(
      process.argv[3],
      parseInt(process.argv[4]),
      parseInt(process.argv[5])
    );
    break;
  case 2:
    if(process.argv.length < 7) exitError();
    updateColor(
      process.argv[3],
      parseInt(process.argv[4]),
      parseInt(process.argv[5]),
      parseInt(process.argv[6])
    );
    break;
  case 3:
    if(process.argv.length < 7) exitError();
    updateTeamColor(
      parseInt(process.argv[3]),
      parseInt(process.argv[4]),
      parseInt(process.argv[5]),
      parseInt(process.argv[6])
    );
    break;
  case 4:
    if(process.argv.length < 5) exitError();
    updateName(process.argv[3], process.argv[4]);
    break;
  case 5:
    if(process.argv.length < 4) exitError();
    sendEcho(process.argv[3]);
    break;
  case 6:
    if(process.argv.length < 10) exitError();
    var address = process.argv[3];
    updateTeam (address, parseInt(process.argv[4]), parseInt(process.argv[5]));
    updateColor(address, parseInt(process.argv[6]), parseInt(process.argv[7]), parseInt(process.argv[8]));
    updateName (address, process.argv[9]);
    break;
}