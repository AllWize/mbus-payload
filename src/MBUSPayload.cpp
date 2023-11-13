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
  int32_t scaled = round(value);
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
    uint8_t difLeast4bit = (dif & 0x0F);
    uint8_t len = 0;
    uint8_t dataCodingType = 0;
    /*
    0 -->   no Data
    1 -->   integer
    2 -->   bcd
    3 -->   real
    4 -->   variable lengs
    5 -->   special functions
    6 -->   TimePoint Date&Time Typ F
    7 -->   TimePoint Date Typ G

    Length in Bit	Code	    Meaning	        Code	Meaning
    0	            0000	    No data	        1000	Selection for Readout
    8	            0001	    8 Bit Integer	1001	2 digit BCD
    16	            0010	    16 Bit Integer	1010	4 digit BCD
    24	            0011	    24 Bit Integer	1011	6 digit BCD
    32	            0100	    32 Bit Integer	1100	8 digit BCD
    32 / N	        0101	    32 Bit Real	    1101	variable length
    48	            0110	    48 Bit Integer	1110	12 digit BCD
    64	            0111	    64 Bit Integer	1111	Special Functions 
    */
    
    switch(difLeast4bit){
        case 0x00:  //No data
            len = 0;
            dataCodingType = 0;
            break;        
        case 0x01:  //0001	    8 Bit Integer
            len = 1;
            dataCodingType = 1;
            break;
        case 0x02:  //0010	    16 Bit Integer
            len = 2;
            dataCodingType = 1;
            break;
        case 0x03:  //0011	    24 Bit Integer
            len = 3;
            dataCodingType = 1;
            break;
        case 0x04:  //0100	    32 Bit Integer
            len = 4;
            dataCodingType = 1;
            break;
        case 0x05:  //0101	    32 Bit Real
            len = 4;
            dataCodingType = 3;
            break;        
        case 0x06:  //0110	    48 Bit Integer
            len = 6;
            dataCodingType = 1;
            break; 
        case 0x07:  //0111	    64 Bit Integer
            len = 8;
            dataCodingType = 1;
            break;
        case 0x08:  //not supported
            len = 0;
            dataCodingType = 0;
            break;
        case 0x09:  //1001 2 digit BCD
            len = 1;
            dataCodingType = 2;
            break;
        case 0x0A:  //1010 4 digit BCD
            len = 2;
            dataCodingType = 2;
            break;
        case 0x0B:  //1011 6 digit BCD
            len = 3;
            dataCodingType = 2;
            break;
        case 0x0C:  //1100 8 digit BCD
            len = 4;
            dataCodingType = 2;
            break;
        case 0x0D:  //1101	variable length
            len = 0;
            dataCodingType = 4;
            break;
        case 0x0E:  //1110	12 digit BCD
            len = 6;
            dataCodingType = 2;
            break;
        case 0x0F:  //1111	Special Functions
            len = 0;
            dataCodingType = 5;
            break;

    }
    
    // handle DIFE to prevent stumble if a DIFE is used
    bool dife = ((dif & 0x80) == 0x80); //check if the first bit of DIF marked as "DIFE is following" 
    while(dife) {
      index++;
      dife = false;
      dife = ((buffer[index-1] & 0x80) == 0x80); //check if after the DIFE another DIFE is following 
      } 
    //  End of DIFE handling
 

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
    
    if((vif & 0x6D) == 0x6D){  // TimePoint Date&Time TypF
        dataCodingType = 6;   
    }
    else if((vif & 0x6C) == 0x6C){  // TimePoint Date TypG
        dataCodingType = 7;   
    }	  

    // Check buffer overflow
    if (index + len > size) {
        _error = MBUS_ERROR::BUFFER_OVERFLOW;
        return 0;
    }

    // read value
    int16_t value16 = 0;  	// int16_t to notice negative values at 2 byte data
    int32_t value32 = 0;	// int32_t to notice negative values at 4 byte data	  
    int64_t value = 0;

    float valueFloat = 0;
	  
	  
	uint8_t date[len] ={0};
	char datestring[12] = {0};
	char datestring2[24] = {0};
	int out_len = 0;	  

    switch(dataCodingType){
        case 0:    //no Data
            
            break;
        case 1:    //integer
            if(len==2){
                for (uint8_t i = 0; i<len; i++) {
                    value16 = (value16 << 8) + buffer[index + len - i - 1];
                }
                value = (int64_t)value16;
            }
            else if(len==4){
                for (uint8_t i = 0; i<len; i++) {
                    value32 = (value32 << 8) + buffer[index + len - i - 1];
                }
                value = (int64_t)value32;
            }			
            else{
                for (uint8_t i = 0; i<len; i++) {
                    value = (value << 8) + buffer[index + len - i - 1];
                   }            
            }
            break;
        case 2:    //bcd
             if(len==2){
                for (uint8_t i = 0; i<len; i++) {
                    uint8_t byte = buffer[index + len - i - 1];
                    value16 = (value16 * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
                }
                value = (int64_t)value16;
			 }
             else if(len==4){
                for (uint8_t i = 0; i<len; i++) {
                    uint8_t byte = buffer[index + len - i - 1];
                    value32 = (value32 * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
                }
                value = (int64_t)value32;				 
            }
            else{
                for (uint8_t i = 0; i<len; i++) {
                    uint8_t byte = buffer[index + len - i - 1];
                    value = (value * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
                }           
            }       
            break;
        case 3:    //real
            for (uint8_t i = 0; i<len; i++) {
                value = (value << 8) + buffer[index + len - i - 1];
            } 
            memcpy(&valueFloat, &value, sizeof(valueFloat));
            break;
        case 4:    //variable lengs
        
            break;
        case 5:    //special functions
        
            break;
        case 6:    //TimePoint Date&Time Typ F

			for (uint8_t i = 0; i<len; i++) {
            	date[i] =  buffer[index + i];
			}            
			
            if ((date[0] & 0x80) != 0) {    // Time valid ?
                //out_len = snprintf(output, output_size, "invalid");
                break;
            }
            out_len = snprintf(datestring, 24, "%02d%02d%02d%02d%02d",
                ((date[2] & 0xE0) >> 5) | ((date[3] & 0xF0) >> 1), // year
                date[3] & 0x0F, // mon
                date[2] & 0x1F, // mday
                date[1] & 0x1F, // hour
            	date[0] & 0x3F // sec
			);  
			
            out_len = snprintf(datestring2, 24, "%02d-%02d-%02dT%02d:%02d:00",
                ((date[2] & 0xE0) >> 5) | ((date[3] & 0xF0) >> 1), // year
                date[3] & 0x0F, // mon
                date[2] & 0x1F, // mday
                date[1] & 0x1F, // hour
                date[0] & 0x3F // sec
            );	
			value = atof( datestring);
            break;
        case 7:    //TimePoint Date Typ G
			for (uint8_t i = 0; i<len; i++) {
            	date[i] =  buffer[index + i];
			}            
			
           if ((date[1] & 0x0F) > 12) {    // Time valid ?
                //out_len = snprintf(output, output_size, "invalid");
                break;
            }
            out_len = snprintf(datestring, 10, "%02d%02d%02d",
                ((date[0] & 0xE0) >> 5) | ((date[1] & 0xF0) >> 1), // year
                date[1] & 0x0F, // mon
                date[0] & 0x1F  // mday
            );
            out_len = snprintf(datestring2, 10, "%02d-%02d-%02d",
                ((date[0] & 0xE0) >> 5) | ((date[1] & 0xF0) >> 1), // year
                date[1] & 0x0F, // mon
                date[0] & 0x1F  // mday
            );			
			value = atof( datestring);
            break;
		default:
			break;
    }

    index += len;

    // scaled value
    double scaled = 0;
	int8_t scalar = vif_defs[def].scalar + vif - vif_defs[def].base;	  
    if(dataCodingType == 3){
        scaled = valueFloat;
    }
    else{
        scaled = value;
        for (int8_t i=0; i<scalar; i++) scaled *= 10;
        for (int8_t i=scalar; i<0; i++) scaled /= 10;
    }
    // Init object
    JsonObject data = root.createNestedObject();
    data["vif"] = vif;
    data["code"] = vif_defs[def].code;
    data["scalar"] = scalar;
    data["value_raw"] = value;
    data["value_scaled"] = scaled;
    data["units"] = String(getCodeUnits(vif_defs[def].code));
    data["name"] = String(getCodeName(vif_defs[def].code));
	data["date"] = String(datestring2);  
  
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

    case MBUS_CODE::TIME_POINT_DATE:
      return "Date_JJMMDD";  

    case MBUS_CODE::TIME_POINT_DATETIME:
      return "Time_JJMMDDhhmm";  

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

    case MBUS_CODE::UNSUPPORTED_X:
      return "X";      

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

    case MBUS_CODE::TIME_POINT_DATE:
    case MBUS_CODE::TIME_POINT_DATETIME:
      return "time_point";

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

    case MBUS_CODE::SOFTWARE_VERSION: 
      return "software_version"; //neu

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

    case MBUS_CODE::UNSUPPORTED_X: 
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
