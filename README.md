# Simple MBus Application Payload Encoder / Decoder

[![version](https://img.shields.io/badge/version-0.1.0-brightgreen.svg)](CHANGELOG.md)
[![travis](https://travis-ci.com/AllWize/mbus-payload.svg?branch=master)](https://travis-ci.com/AllWize/mbus-payload)
[![codacy](https://img.shields.io/codacy/grade/9cc6a9ad619d4c539866300670103f5d/master.svg)](https://www.codacy.com/app/AllWize/mbus-payload/dashboard)
[![license](https://img.shields.io/badge/license-LGPL--3.0-orange.svg)](LICENSE)

## Documentation

The `MBUSPayload` class enables Arduino devices to encode data with using the MBUS DIF/VIF payload protocol.

### Class: `MBUSPayload`

Include and instantiate the MBUSPayload class. The constructor takes the size of the allocated buffer. Depending on the LoRa frequency plan and data rate used, the maximum payload varies, the default is 32 bytes.

```c
#include <MBUSPayload.h>

MBUSPayload payload(uint8_t size);
```

- `uint8_t size`: The maximum payload size to send, e.g. `32`.

### Example

```c
MBUSPayload payload();
payload.reset();
payload.addField(MBUS_CODE_EXTERNAL_TEMPERATURE_C, 22.5);
wize.send(payload.getBuffer(), payload.getSize());
```

### Method: `reset`

Resets the buffer.

```c
void reset(void);
```

### Method: `getSize`

Returns the size of the buffer.

```c
uint8_t getSize(void);
```

### Method: `getBuffer`

Returns a pointer to the buffer.

```c
uint8_t *getBuffer(void);
```

### Method: `copy`

Copies the internal buffer to a specified buffer and returns the copied size.

```c
uint8_t copy(uint8_t *buffer);
```

### Method: `addRaw`

Adds a data field to the buffer. It expect the raw DIF, VIF and data values. Please notice that DIFE fields are not supported yet.

Returns the final position in the buffer if OK, else returns 0.

```c
uint8_t addRaw(uint8_t dif, uint32_t vif, uint32_t value);
```

### Method: `addField`

Adds a data field to the buffer. It expects the type code and the value as a real number and will calculate the best DIF and VIF to hold the data. ALternatively you can provide the scalar and the value as an integer to force a certain scale.

Returns the final position in the buffer if OK, else returns 0.

```c
uint8_t addField(uint8_t code, float value);
uint8_t addField(uint8_t code, int8_t scalar, uint32_t value);
```

Supported codes:

|Domain|Codes|
|---|---|
|Energy|MBUS_CODE_ENERGY_WH, MBUS_CODE_ENERGY_J|
|Electricity|MBUS_CODE_VOLTS, MBUS_CODE_AMPERES|
|Power|MBUS_CODE_POWER_W, MBUS_CODE_POWER_J_H, MBUS_CODE_MAX_POWER_W|
|Volume|MBUS_CODE_VOLUME_M3, MBUS_CODE_VOLUME_FT3, MBUS_CODE_VOLUME_GAL|
|Mass|MBUS_CODE_MASS_KG|
|Temperature|MBUS_CODE_FLOW_TEMPERATURE_C, MBUS_CODE_RETURN_TEMPERATURE_C, MBUS_CODE_TEMPERATURE_DIFF_K, MBUS_CODE_EXTERNAL_TEMPERATURE_C,<br /> MBUS_CODE_FLOW_TEMPERATURE_F, MBUS_CODE_RETURN_TEMPERATURE_F, MBUS_CODE_TEMPERATURE_DIFF_F, MBUS_CODE_EXTERNAL_TEMPERATURE_F,<br /> MBUS_CODE_TEMPERATURE_LIMIT_F, MBUS_CODE_TEMPERATURE_LIMIT_C|
|Pressure|MBUS_CODE_PRESSURE_BAR|
|Flow|MBUS_CODE_VOLUME_FLOW_M3_H, MBUS_CODE_VOLUME_FLOW_M3_MIN, MBUS_CODE_VOLUME_FLOW_M3_S, MBUS_CODE_MASS_FLOW_KG_H,<br />MBUS_CODE_VOLUME_FLOW_GAL_M, MBUS_CODE_VOLUME_FLOW_GAL_H|
|Time|MBUS_CODE_ON_TIME_S, MBUS_CODE_ON_TIME_MIN, MBUS_CODE_ON_TIME_H, MBUS_CODE_ON_TIME_DAYS<br />MBUS_CODE_OPERATING_TIME_S, MBUS_CODE_OPERATING_TIME_MIN, MBUS_CODE_OPERATING_TIME_H, MBUS_CODE_OPERATING_TIME_DAYS, <br />MBUS_CODE_AVG_DURATION_S, MBUS_CODE_AVG_DURATION_MIN, MBUS_CODE_AVG_DURATION_H, MBUS_CODE_AVG_DURATION_DAYS,<br />MBUS_CODE_ACTUAL_DURATION_S, MBUS_CODE_ACTUAL_DURATION_MIN, MBUS_CODE_ACTUAL_DURATION_H, MBUS_CODE_ACTUAL_DURATION_DAYS|
|Currency|MBUS_CODE_CREDIT, MBUS_CODE_DEBIT|
|Device|MBUS_CODE_FABRICATION_NUMBER, MBUS_CODE_BUS_ADDRESS, MBUS_CODE_ACCESS_NUMBER,<br />MBUS_CODE_MANUFACTURER, MBUS_CODE_MODEL_VERSION, MBUS_CODE_HARDWARE_VERSION, MBUS_CODE_FIRMWARE_VERSION|
|Generic|MBUS_CODE_GENERIC|
|Other|MBUS_CODE_CUSTOMER, MBUS_CODE_ERROR_FLAGS, MBUS_CODE_ERROR_MASK, MBUS_CODE_DIGITAL_OUTPUT, MBUS_CODE_DIGITAL_INPUT,<br />MBUS_CODE_BAUDRATE_BPS, MBUS_CODE_RESPONSE_DELAY_TIME, MBUS_CODE_RETRY,<br />MBUS_CODE_RESET_COUNTER, MBUS_CODE_CUMULATION_COUNTER|

### Method: `decode`

Decodes a byte array into a JsonArray (requires ArduinoJson library). The result is an array of objects, each one containing channel, type, type name and value. The value can be a scalar or an object (for accelerometer, gyroscope and GPS data). The method call returns the number of decoded fields or 0 if error.

```c
uint8_t decode(uint8_t *buffer, uint8_t size, JsonArray& root);
```

Example output:

```
[
  {
    "vif": 64257,
    "code": 0,
    "scalar": 6,
    "value_raw": 200,
    "value_scaled": 2e8
  }
]
```

### Method: `getCodeUnits`

Returns a pointer to a C-string with the unit abbreviation for the given code.


```c
const char * getCodeUnits(uint8_t code);
```

### Method: `getError`

Returns the last error ID, once returned the error is reset to OK. Possible error values are:

* `MBUS_ERROR_OK`: No error
* `MBUS_ERROR_OVERFLOW`: Buffer cannot hold the requested data, try increasing the buffer size. When decoding: incomming buffer size is wrong.
* `MBUS_ERROR_UNSUPPORTED_CODING`: The library only supports 1,2,3 and 4 bytes integers and 2,4,6 or 8 BCD.
* `MBUS_ERROR_UNSUPPORTED_RANGE`: Couldn't encode the provided combination of code and scale, try changing the scale of your value.
* `MBUS_ERROR_UNSUPPORTED_VIF`: When decoding: the VIF is not supported and thus it cannot be decoded.
* `MBUS_ERROR_NEGATIVE_VALUE`: Library only supports non-negative values at the moment.

```c
uint8_t getError(void);
```

## References

* [The M-Bus: A Documentation Rev. 4.8 - Appendix](http://www.m-bus.com/mbusdoc/md8.php)
* [Dedicated Application Layer (M-Bus)](http://www.m-bus.com/files/w4b21021.pdf) by H. Ziegler

## License

Copyright (C) 2019 by AllWize<br />
Copyright (C) 2019 by Xose Pérez <xose at allwize dot io>

The MBUSPayload library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The MBUSPayload library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MBUSPayload library.  If not, see <http://www.gnu.org/licenses/>.
