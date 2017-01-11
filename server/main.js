var beacon = require("eddystone-beacon");
var util = require('util');
var bleno = require('bleno');

DEVICE_NAME = 'Edison';

process.env['BLENO_DEVICE_NAME'] = DEVICE_NAME;

var FingerprintCharacteristic = function() {
  bleno.Characteristic.call(this, {
    uuid: 'fc0a',
    properties: ['read'],
    value: null
  });
  this._value = new Buffer([0x89,0x88,0x07,0x3b,0x0c,0x73,0x02,0x5c,0x23,0x26,0xbb,0xbe,0x06,0xcd,0x71,0xfc,0x9f,0x3c,0xe3,0x5c,0x68,0xc5,0x25,0xec,0x28,0x57,0x22,0x1b,0x19,0x83,0x8f]);

};

util.inherits(FingerprintCharacteristic, bleno.Characteristic);

FingerprintCharacteristic.prototype.onReadRequest = function(offset, callback) {
  console.log('reading fingerprint');

  callback(this.RESULT_SUCCESS, this._value);
};


var NonceCharacteristic = function() {
  bleno.Characteristic.call(this, {
    uuid: 'fc0b',
    properties: ['read'],
    value: null
  });
  this._value = new Buffer([0xff,0xfe,0xfd,0xfb,0xfa,0xf9,0xf8,0xf7,0xf6,0xf5,0xf4,0xf3,0xf2,0xf1,0xf0]);
};

util.inherits(NonceCharacteristic, bleno.Characteristic);

NonceCharacteristic.prototype.onReadRequest = function(offset, callback) {
  console.log('reading Nonce');

  callback(this.RESULT_SUCCESS, this._value);
};

console.log("waiting for bleno");

bleno.on('stateChange', function(state) {
  console.log(state);

  if (state === 'poweredOn') {
    bleno.startAdvertising(DEVICE_NAME, ['fc00']);
    beacon.advertiseUrl("https://pi.pe/iot/bl.html", {name: DEVICE_NAME});
  } else {
    if (state === 'unsupported'){
      console.log("BLE and Bleno configurations not enabled on board");
    }
    bleno.stopAdvertising();
  }
});

var fingerprintCharacteristic = new FingerprintCharacteristic();
var nonceCharacteristic = new NonceCharacteristic();


bleno.on('advertisingStart', function(error) {
  console.log('advertisingStart: ' + (error ? error : 'success'));

  if (error) {
    return;
  }
  console.log('setting service');

  bleno.setServices([
    new bleno.PrimaryService({
      uuid: 'fc00',
      characteristics: [
        fingerprintCharacteristic,
        nonceCharacteristic
      ]
    })
  ]);
});

bleno.on('accept', function(clientAddress) {
  console.log("Accepted Connection: " + clientAddress);
});

bleno.on('disconnect', function(clientAddress) {
  console.log("Disconnected Connection: " + clientAddress);
});




