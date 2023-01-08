/*

MBUS Payload Encoder / Decoder

Copyright (C) 2019 by AllWize
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

*/

#include "MBUSPayload.h"

// ----------------------------------------------------------------------------

MBUSPayload::MBUSPayload(uint8_t size) : _maxsize(size) {
  _buffer = (uint8_t *) malloc(size);
  _cursor = 0;
}

MBUSPayload::~MBUSPayload(void) {
  free(_buffer);
}

void MBUSPayload::reset(void) {
  _cursor = 0;
}

uint8_t MBUSPayload::getSize(void) {
  return _cursor;
}

uint8_t *MBUSPayload::getBuffer(void) {
  return _buffer;
}

uint8_t MBUSPayload::copy(uint8_t *dst) {
  memcpy(dst, _buffer, _cursor);
  return _cursor;
}

uint8_t MBUSPayload::getError() {
  uint8_t error = _error;
  _error = MBUS_ERROR::NO_ERROR;
  return error;
}

// ----------------------------------------------------------------------------

uint8_t MBUSPayload::addRaw(uint8_t dif, uint32_t vif, uint32_t value) {

    // Check supported codings (1 to 4 bytes o 2-8 BCD)
    bool bcd = ((dif & 0x08) == 0x08);
    uint8_t len = (dif & 0x07);
    if ((len < 1) || (4 < len)) {
      _error = MBUS_ERROR::UNSUPPORTED_CODING;
      return 0;
    }

    // Calculate VIF(E) size
    uint8_t vif_len = 0;
    uint32_t vif_copy = vif;
    while (vif_copy > 0) {
      vif_len++;
      vif_copy >>= 8;
    }

    // Check buffer overflow
    if ((_cursor + 1 + vif_len + len) > _maxsize) {
      _error = MBUS_ERROR::BUFFER_OVERFLOW;
      return 0;
    }

    // Store DIF
    _buffer[_cursor++] = dif;

    // Store VIF
    for (uint8_t i = 0; i<vif_len; i++) {
      _buffer[_cursor + vif_len - i - 1] = (vif & 0xFF);
      vif >>= 8;
    }
    _cursor += vif_len;

    // Value Information Block - Data
    if (bcd) {
      for (uint8_t i = 0; i<len; i++) {
        _buffer[_cursor++] = ((value / 10) % 10) * 16 + (value % 10);
        value = value / 100;
      }
    } else {
      for (uint8_t i = 0; i<len; i++) {
        _buffer[_cursor++] = value & 0xFF;
        value >>= 8;
      }
    }

    return _cursor;

}

uint8_t MBUSPayload::addField(uint8_t code, int8_t scalar, uint32_t value) {

  // Find the closest code-scalar match
  uint32_t vif = _getVIF(code, scalar);
  if (0xFF == vif) {
    _error = MBUS_ERROR::UNSUPPORTED_RANGE;
    return 0;
  }

  // Calculate coding length
  uint32_t copy = value >> 8;
  uint8_t coding = 1;
  while (copy > 0) {
    copy >>= 8;
    coding++;
  }
  if (coding > 4) {
    coding = 4;
  }

  // Add value
  return addRaw(coding, vif, value);

}

uint8_t MBUSPayload::addField(uint8_t code, float value) {

  // Does not support negative values
  if (value < 0) {
    _error = MBUS_ERROR::NEGATIVE_VALUE;
    return 0;
  }

  // Special case fot value == 0
  if (value < ARDUINO_FLOAT_MIN) {
    return addField(code, 0, value);
  }

  // Get the size of the integer part
  int8_t int_size = 0;
  uint32_t tmp = value;
  while (tmp > 10) {
    tmp /= 10;
    int_size++;
  }

  // Calculate scale
  int8_t scalar = 0;

  // If there is a fractional part, move up 8-int_size positions
  float frac = value - int(value);
  if (frac > ARDUINO_FLOAT_MIN) {
    scalar = int_size - ARDUINO_FLOAT_DECIMALS; 
    for (int8_t i=scalar; i<0; i++) {
      value *= 10.0;
    }
  }

  // Check validity when no decimals
  bool valid = (_getVIF(code, scalar) != 0xFF);

  // Now move down 
  uint32_t scaled = round(value);
  while ((scaled % 10) == 0) {
    scalar++;
    scaled /= 10;
    if (_getVIF(code, scalar) == 0xFF) {
      if (valid) {
        scalar--;
        scaled *= 10;
        break;
      }
    } else {
      valid = true;
    }
  }
  
  // Convert to integer
  return addField(code, scalar, scaled);

}

uint8_t MBUSPayload::decode(uint8_t *buffer, uint8_t size, JsonArray& root) {

  uint8_t count = 0;
  uint8_t index = 0;

  while (index < size) {

    count++;

    // Decode DIF
    uint8_t dif = buffer[index++];
    bool bcd = ((dif & 0x08) == 0x08);
    uint8_t len = (dif & 0x07);
    if ((len < 1) || (4 < len)) {
      _error = MBUS_ERROR::UNSUPPORTED_CODING;
      return 0;
    }
    
    // Get VIF(E)
    uint32_t vif = 0;
    do {
      if (index == size) {
        _error = MBUS_ERROR::BUFFER_OVERFLOW;
        return 0;
      }
      vif = (vif << 8) + buffer[index++];
    } while ((vif & 0x80) == 0x80);

    // Find definition
    int8_t def = _findDefinition(vif);
    if (def < 0) {
      _error = MBUS_ERROR::UNSUPPORTED_VIF;
      return 0;
    }

    // Check buffer overflow
    if (index + len > size) {
        _error = MBUS_ERROR::BUFFER_OVERFLOW;
        return 0;
    }

    // read value
    uint32_t value = 0;
    if (bcd) {
      for (uint8_t i = 0; i<len; i++) {
        uint8_t byte = buffer[index + len - i - 1];
        value = (value * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
      }
    } else {
      for (uint8_t i = 0; i<len; i++) {
        value = (value << 8) + buffer[index + len - i - 1];
      }
    }
    index += len;

    // scaled value
    int8_t scalar = vif_defs[def].scalar + vif - vif_defs[def].base;
    double scaled = value;
    for (int8_t i=0; i<scalar; i++) scaled *= 10;
    for (int8_t i=scalar; i<0; i++) scaled /= 10;

    // Init object
    JsonObject data = root.createNestedObject();
    data["vif"] = vif;
    data["code"] = vif_defs[def].code;
    data["scalar"] = scalar;
    data["value_raw"] = value;
    data["value_scaled"] = scaled;
    //data["units"] = String(getCodeUnits(vif_defs[def].code));
  
  }

  return count;

}

const char * MBUSPayload::getCodeUnits(uint8_t code) {
  switch (code) {

    case MBUS_CODE::ENERGY_WH:
      return "Wh";
    
    case MBUS_CODE::ENERGY_J:
      return "J";

    case MBUS_CODE::VOLUME_M3: 
      return "m3";

    case MBUS_CODE::MASS_KG: 
      return "s";

    case MBUS_CODE::ON_TIME_S: 
    case MBUS_CODE::OPERATING_TIME_S: 
    case MBUS_CODE::AVG_DURATION_S:
    case MBUS_CODE::ACTUAL_DURATION_S:
      return "s";

    case MBUS_CODE::ON_TIME_MIN: 
    case MBUS_CODE::OPERATING_TIME_MIN: 
    case MBUS_CODE::AVG_DURATION_MIN:
    case MBUS_CODE::ACTUAL_DURATION_MIN:
      return "min";
      
    case MBUS_CODE::ON_TIME_H: 
    case MBUS_CODE::OPERATING_TIME_H: 
    case MBUS_CODE::AVG_DURATION_H:
    case MBUS_CODE::ACTUAL_DURATION_H:
      return "h";
      
    case MBUS_CODE::ON_TIME_DAYS: 
    case MBUS_CODE::OPERATING_TIME_DAYS: 
    case MBUS_CODE::AVG_DURATION_DAYS:
    case MBUS_CODE::ACTUAL_DURATION_DAYS:
      return "days";
      
    case MBUS_CODE::POWER_W:
    case MBUS_CODE::MAX_POWER_W: 
      return "W";
      
    case MBUS_CODE::POWER_J_H: 
      return "J/h";
      
    case MBUS_CODE::VOLUME_FLOW_M3_H: 
      return "m3/h";
      
    case MBUS_CODE::VOLUME_FLOW_M3_MIN:
      return "m3/min";
      
    case MBUS_CODE::VOLUME_FLOW_M3_S: 
      return "m3/s";
      
    case MBUS_CODE::MASS_FLOW_KG_H: 
      return "kg/h";
      
    case MBUS_CODE::FLOW_TEMPERATURE_C: 
    case MBUS_CODE::RETURN_TEMPERATURE_C: 
    case MBUS_CODE::EXTERNAL_TEMPERATURE_C: 
    case MBUS_CODE::TEMPERATURE_LIMIT_C:
      return "C";

    case MBUS_CODE::TEMPERATURE_DIFF_K: 
      return "K";

    case MBUS_CODE::PRESSURE_BAR: 
      return "bar";

    case MBUS_CODE::BAUDRATE_BPS:
      return "bps";

    case MBUS_CODE::VOLTS: 
      return "V";

    case MBUS_CODE::AMPERES: 
      return "A";
      
    case MBUS_CODE::VOLUME_FT3:
      return "ft3";

    case MBUS_CODE::VOLUME_GAL: 
      return "gal";
      
    case MBUS_CODE::VOLUME_FLOW_GAL_M: 
      return "gal/min";
      
    case MBUS_CODE::VOLUME_FLOW_GAL_H: 
      return "gal/h";
      
    case MBUS_CODE::FLOW_TEMPERATURE_F:
    case MBUS_CODE::RETURN_TEMPERATURE_F:
    case MBUS_CODE::TEMPERATURE_DIFF_F:
    case MBUS_CODE::EXTERNAL_TEMPERATURE_F:
    case MBUS_CODE::TEMPERATURE_LIMIT_F:
      return "F";

    default:
      break; 

  }

  return "";

}

const char * MBUSPayload::getCodeName(uint8_t code) {
  switch (code) {

    case MBUS_CODE::ENERGY_WH:
    case MBUS_CODE::ENERGY_J:
      return "energy";
    
    case MBUS_CODE::VOLUME_M3: 
    case MBUS_CODE::VOLUME_FT3:
    case MBUS_CODE::VOLUME_GAL: 
      return "volume";

    case MBUS_CODE::MASS_KG: 
      return "mass";

    case MBUS_CODE::ON_TIME_S: 
    case MBUS_CODE::ON_TIME_MIN: 
    case MBUS_CODE::ON_TIME_H: 
    case MBUS_CODE::ON_TIME_DAYS: 
      return "on_time";
    
    case MBUS_CODE::OPERATING_TIME_S: 
    case MBUS_CODE::OPERATING_TIME_MIN: 
    case MBUS_CODE::OPERATING_TIME_H: 
    case MBUS_CODE::OPERATING_TIME_DAYS: 
      return "operating_time";
    
    case MBUS_CODE::AVG_DURATION_S:
    case MBUS_CODE::AVG_DURATION_MIN:
    case MBUS_CODE::AVG_DURATION_H:
    case MBUS_CODE::AVG_DURATION_DAYS:
      return "avg_duration";
    
    case MBUS_CODE::ACTUAL_DURATION_S:
    case MBUS_CODE::ACTUAL_DURATION_MIN:
    case MBUS_CODE::ACTUAL_DURATION_H:
    case MBUS_CODE::ACTUAL_DURATION_DAYS:
      return "actual_duration";

    case MBUS_CODE::POWER_W:
    case MBUS_CODE::MAX_POWER_W: 
    case MBUS_CODE::POWER_J_H: 
      return "power";
      
    case MBUS_CODE::VOLUME_FLOW_M3_H: 
    case MBUS_CODE::VOLUME_FLOW_M3_MIN:
    case MBUS_CODE::VOLUME_FLOW_M3_S: 
    case MBUS_CODE::VOLUME_FLOW_GAL_M: 
    case MBUS_CODE::VOLUME_FLOW_GAL_H: 
      return "volume_flow";

    case MBUS_CODE::MASS_FLOW_KG_H: 
      return "mass_flow";

    case MBUS_CODE::FLOW_TEMPERATURE_C: 
    case MBUS_CODE::FLOW_TEMPERATURE_F:
      return "flow_temperature";

    case MBUS_CODE::RETURN_TEMPERATURE_C: 
    case MBUS_CODE::RETURN_TEMPERATURE_F:
      return "return_temperature";

    case MBUS_CODE::EXTERNAL_TEMPERATURE_C: 
    case MBUS_CODE::EXTERNAL_TEMPERATURE_F:
      return "external_temperature";

    case MBUS_CODE::TEMPERATURE_LIMIT_C:
    case MBUS_CODE::TEMPERATURE_LIMIT_F:
      return "temperature_limit";

    case MBUS_CODE::TEMPERATURE_DIFF_K: 
    case MBUS_CODE::TEMPERATURE_DIFF_F:
      return "temperature_diff";

    case MBUS_CODE::PRESSURE_BAR: 
      return "pressure";

    case MBUS_CODE::BAUDRATE_BPS:
      return "baudrate";

    case MBUS_CODE::VOLTS: 
      return "voltage";

    case MBUS_CODE::AMPERES: 
      return "current";
      
    case MBUS_CODE::FABRICATION_NUMBER: 
      return "fab_number";

    case MBUS_CODE::BUS_ADDRESS: 
      return "bus_address";

    case MBUS_CODE::CREDIT: 
      return "credit";

    case MBUS_CODE::DEBIT: 
      return "debit";

    case MBUS_CODE::ACCESS_NUMBER: 
      return "access_number";

    case MBUS_CODE::MANUFACTURER: 
      return "manufacturer";

    case MBUS_CODE::MODEL_VERSION: 
      return "model_version";

    case MBUS_CODE::HARDWARE_VERSION: 
      return "hardware_version";

    case MBUS_CODE::FIRMWARE_VERSION: 
      return "firmware_version";

    case MBUS_CODE::CUSTOMER: 
      return "customer";
  
    case MBUS_CODE::ERROR_FLAGS: 
      return "error_flags";
  
    case MBUS_CODE::ERROR_MASK: 
      return "error_mask";
  
    case MBUS_CODE::DIGITAL_OUTPUT: 
      return "digital_output";
  
    case MBUS_CODE::DIGITAL_INPUT: 
      return "digital_input";
  
    case MBUS_CODE::RESPONSE_DELAY_TIME: 
      return "response_delay";
  
    case MBUS_CODE::RETRY: 
      return "retry";
  
    case MBUS_CODE::GENERIC: 
      return "generic";
  
    case MBUS_CODE::RESET_COUNTER: 
    case MBUS_CODE::CUMULATION_COUNTER: 
      return "counter";
  
    default:
        break; 

  }

  return "";

}
// ----------------------------------------------------------------------------

int8_t MBUSPayload::_findDefinition(uint32_t vif) {
  
  for (uint8_t i=0; i<MBUS_VIF_DEF_NUM; i++) {
    vif_def_type vif_def = vif_defs[i];
    if ((vif_def.base <= vif) && (vif < (vif_def.base + vif_def.size))) {
      return i;
    }
  }
  
  return -1;

}

uint32_t MBUSPayload::_getVIF(uint8_t code, int8_t scalar) {

  for (uint8_t i=0; i<MBUS_VIF_DEF_NUM; i++) {
    vif_def_type vif_def = vif_defs[i];
    if (code == vif_def.code) {
      if ((vif_def.scalar <= scalar) && (scalar < (vif_def.scalar + vif_def.size))) {
        return vif_def.base + (scalar - vif_def.scalar);
      }
    }
  }
  
  return 0xFF; // this is not a valid VIF

}