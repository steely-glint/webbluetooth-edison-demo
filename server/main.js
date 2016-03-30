var five = require("johnny-five");
var Edison = require("edison-io");
var board = new five.Board({
  io: new Edison()
});

var beacon = require("eddystone-beacon");
var util = require('util');
var bleno = require('eddystone-beacon/node_modules/bleno');
var parse = require('parse-color');

DEVICE_NAME = 'Edison';

process.env['BLENO_DEVICE_NAME'] = DEVICE_NAME;

var TemperatureCharacteristic = function() {
  bleno.Characteristic.call(this, {
    uuid: 'fc0a',
    properties: ['read', 'notify'],
    value: null
  });

  this._lastValue = 0;
  this._total = 0;
  this._samples = 0;
  this._onChange = null;
};

util.inherits(TemperatureCharacteristic, bleno.Characteristic);

TemperatureCharacteristic.prototype.onReadRequest = function(offset, callback) {
  var data = new Buffer(8);
  data.writeDoubleLE(this._lastValue, 0);
  callback(this.RESULT_SUCCESS, data);
};

TemperatureCharacteristic.prototype.onSubscribe = function(maxValueSize, updateValueCallback) {
  console.log("Subscribed to temperature change.");
  this._onChange = updateValueCallback;
  this._lastValue = undefined;
};

TemperatureCharacteristic.prototype.onUnsubscribe = function() {
  console.log("Unsubscribed to temperature change.");
  this._onChange = null;
};

var NO_SAMPLES = 100;

TemperatureCharacteristic.prototype.valueChange = function(value) {
  this._total += value;
  this._samples++;

  if (this._samples < NO_SAMPLES) {
    return;
  }

  var newValue = Math.round(this._total / NO_SAMPLES);

  this._total = 0;
  this._samples = 0;

  if (this._lastValue && Math.abs(this._lastValue - newValue) < 1) {
    return;
  }

  this._lastValue = newValue;
  console.log("Temperature change " + newValue);

  var data = new Buffer(8);
  data.writeDoubleLE(newValue, 0);

  if (this._onChange) {
    this._onChange(data);
  }
};

var ColorCharacteristic = function() {
  bleno.Characteristic.call(this, {
    uuid: 'fc0b',
    properties: ['read', 'write'],
    value: null
  });
  this._value = 'ffffff';
  this._led = null;
};

util.inherits(ColorCharacteristic, bleno.Characteristic);

ColorCharacteristic.prototype.onReadRequest = function(offset, callback) {
  var data = new Buffer(this._value);
  callback(this.RESULT_SUCCESS, data);
};

ColorCharacteristic.prototype.onWriteRequest = function(data, offset, withoutResponse, callback) {
  var value = parse(data.toString('utf8')).hex;
  if (!value) {
    callback(this.RESULT_SUCCESS);
    return;
  }

  this._value = value;
  console.log(value);

  if (this._led) {
    this._led.color(this._value);
  }
  callback(this.RESULT_SUCCESS);
};

bleno.on('stateChange', function(state) {
  console.log(state);

  if (state === 'poweredOn') {
    bleno.startAdvertising(DEVICE_NAME, ['fc00']);
    beacon.advertiseUrl("https://goo.gl/9FomQC", {name: DEVICE_NAME});
  } else {
    if (state === 'unsupported'){
      console.log("BLE and Bleno configurations not enabled on board");
    }
    bleno.stopAdvertising();
  }
});

var temperatureCharacteristic = new TemperatureCharacteristic();
var colorCharacteristic = new ColorCharacteristic();

bleno.on('advertisingStart', function(error) {
  console.log('advertisingStart: ' + (error ? error : 'success'));

  if (error) {
    return;
  }

  bleno.setServices([
    new bleno.PrimaryService({
      uuid: 'fc00',
      characteristics: [
        temperatureCharacteristic,
        colorCharacteristic
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

board.on("ready", function() {
  // Johnny-Five's Led.RGB class can be initialized with
  // an array of pin numbers in R, G, B order.
  // Reference: http://johnny-five.io/api/led.rgb/#parameters
  var led = new five.Led.RGB([ 3, 5, 6 ]);

  board.analogRead("A0", function(raw) {
    var mV = 5 * 1000 * (raw / 1024);
    var value = (mV / 10) - 50;
    temperatureCharacteristic.valueChange(value);
  });

  colorCharacteristic._led = led;
  led.color(colorCharacteristic._value);
  led.intensity(30);
});
