var beacon = require("eddystone-beacon");
var util = require('util');
var bleno = require('bleno');

DEVICE_NAME = 'Pipe';

process.env['BLENO_DEVICE_NAME'] = DEVICE_NAME;

var ByteCharacteristic = function(uuid,val) {
  bleno.Characteristic.call(this, {
    uuid: uuid,
    properties: ['read'],
    value: null
  });
  this._value = val;

};

util.inherits(ByteCharacteristic, bleno.Characteristic);

ByteCharacteristic.prototype.onReadRequest = function(offset, callback) {
  console.log('reading uuid '+this.uuid);

  callback(this.RESULT_SUCCESS, this._value);
};




console.log("waiting for bleno");

bleno.on('stateChange', function(state) {
  console.log(state);

  if (state === 'poweredOn') {
    bleno.startAdvertising(DEVICE_NAME, ['919e']);
    beacon.advertiseUrl("https://pi.pe/iot/bl.html", {name: DEVICE_NAME});
  } else {
    if (state === 'unsupported'){
      console.log("BLE and Bleno configurations not enabled on board");
    }
    bleno.stopAdvertising();
  }
});

var fingerprintACharacteristic = new ByteCharacteristic('fc0a',new Buffer([0xBD,0xE3,0xCC,0x6C,0xA1,0x7A,0x60,0xB0,0x3C,0x43,0x37,0x36,0x69,0xEE,0x7C,0xA6]));
var fingerprintBCharacteristic = new ByteCharacteristic('fc0b',new Buffer([0x5D,0x67,0xFE,0xFF,0x72,0x92,0x66,0x49,0x4C,0x13,0x11,0x8C,0xEC,0xE2,0xC2,0xA3]));
var nonceCharacteristic = new ByteCharacteristic('fc0c',new Buffer([0xff,0xfe,0xfd,0xfb,0xfa,0xf9,0xf8,0xf7,0xf6,0xf5,0xf4,0xf3,0xf2,0xf1,0xf0]));


bleno.on('advertisingStart', function(error) {
  console.log('advertisingStart: ' + (error ? error : 'success'));

  if (error) {
    return;
  }
  console.log('setting service');

  bleno.setServices([
    new bleno.PrimaryService({
      uuid: '919e',
      characteristics: [
        fingerprintACharacteristic,fingerprintBCharacteristic,
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




