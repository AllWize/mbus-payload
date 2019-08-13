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

#ifndef MBUS_PAYLOAD_H
#define MBUS_PAYLOAD_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define MBUS_DEFAULT_BUFFER_SIZE          32

// Supported code types
enum {

  // no VIFE
  MBUS_CODE_ENERGY_WH, 
  MBUS_CODE_ENERGY_J,
  MBUS_CODE_VOLUME_M3, 
  MBUS_CODE_MASS_KG, 
  MBUS_CODE_ON_TIME_S, 
  MBUS_CODE_ON_TIME_MIN, 
  MBUS_CODE_ON_TIME_H, 
  MBUS_CODE_ON_TIME_DAYS, 
  MBUS_CODE_OPERATING_TIME_S, 
  MBUS_CODE_OPERATING_TIME_MIN, 
  MBUS_CODE_OPERATING_TIME_H, 
  MBUS_CODE_OPERATING_TIME_DAYS, 
  MBUS_CODE_POWER_W,
  MBUS_CODE_POWER_J_H, 
  MBUS_CODE_VOLUME_FLOW_M3_H, 
  MBUS_CODE_VOLUME_FLOW_M3_MIN,
  MBUS_CODE_VOLUME_FLOW_M3_S, 
  MBUS_CODE_MASS_FLOW_KG_H, 
  MBUS_CODE_FLOW_TEMPERATURE_C, 
  MBUS_CODE_RETURN_TEMPERATURE_C, 
  MBUS_CODE_TEMPERATURE_DIFF_K, 
  MBUS_CODE_EXTERNAL_TEMPERATURE_C, 
  MBUS_CODE_PRESSURE_BAR, 
  //MBUS_CODE_TIME_POINT_DATE,
  //MBUS_CODE_TIME_POINT_DATETIME,
  //MBUS_CODE_HCA,
  MBUS_CODE_AVG_DURATION_S,
  MBUS_CODE_AVG_DURATION_MIN,
  MBUS_CODE_AVG_DURATION_H,
  MBUS_CODE_AVG_DURATION_DAYS,
  MBUS_CODE_ACTUAL_DURATION_S,
  MBUS_CODE_ACTUAL_DURATION_MIN,
  MBUS_CODE_ACTUAL_DURATION_H,
  MBUS_CODE_ACTUAL_DURATION_DAYS,
  MBUS_CODE_FABRICATION_NUMBER,
  MBUS_CODE_BUS_ADDRESS,

  // VIFE 0xFD
  MBUS_CODE_CREDIT,
  MBUS_CODE_DEBIT,
  MBUS_CODE_ACCESS_NUMBER,
  //MBUS_CODE_MEDIUM,
  MBUS_CODE_MANUFACTURER,
  //MBUS_CODE_PARAMETER_SET_ID,
  MBUS_CODE_MODEL_VERSION,
  MBUS_CODE_HARDWARE_VERSION,
  MBUS_CODE_FIRMWARE_VERSION,
  //MBUS_CODE_SOFTWARE_VERSION,
  //MBUS_CODE_CUSTOMER_LOCATION,
  MBUS_CODE_CUSTOMER,
  //MBUS_CODE_ACCESS_CODE_USER,
  //MBUS_CODE_ACCESS_CODE_OPERATOR,
  //MBUS_CODE_ACCESS_CODE_SYSOP,
  //MBUS_CODE_ACCESS_CODE_DEVELOPER,
  //MBUS_CODE_PASSWORD,
  MBUS_CODE_ERROR_FLAGS,
  MBUS_CODE_ERROR_MASK,
  MBUS_CODE_DIGITAL_OUTPUT,
  MBUS_CODE_DIGITAL_INPUT,
  MBUS_CODE_BAUDRATE_BPS,
  MBUS_CODE_RESPONSE_DELAY_TIME,
  MBUS_CODE_RETRY,
  MBUS_CODE_GENERIC,
  MBUS_CODE_VOLTS, 
  MBUS_CODE_AMPERES, 
  MBUS_CODE_RESET_COUNTER,
  MBUS_CODE_CUMULATION_COUNTER,

  // VIFE 0xFB
  MBUS_CODE_VOLUME_FT3,
  MBUS_CODE_VOLUME_GAL, 
  MBUS_CODE_VOLUME_FLOW_GAL_M, 
  MBUS_CODE_VOLUME_FLOW_GAL_H, 
  MBUS_CODE_FLOW_TEMPERATURE_F,
  MBUS_CODE_RETURN_TEMPERATURE_F,
  MBUS_CODE_TEMPERATURE_DIFF_F,
  MBUS_CODE_EXTERNAL_TEMPERATURE_F,
  MBUS_CODE_TEMPERATURE_LIMIT_F,
  MBUS_CODE_TEMPERATURE_LIMIT_C,
  MBUS_CODE_MAX_POWER_W,
  
};

// Supported encodings
enum {
  MBUS_CODING_8BIT = 0x01,
  MBUS_CODING_16BIT,
  MBUS_CODING_24BIT,
  MBUS_CODING_32BIT,
  MBUS_CODING_2BCD = 0x09,
  MBUS_CODING_4BCD,
  MBUS_CODING_6BCD,
  MBUS_CODING_8BCD,
};

// Error codes
enum {
  MBUS_ERROR_OK,
  MBUS_ERROR_OVERFLOW,
  MBUS_ERROR_UNSUPPORTED_CODING,
  MBUS_ERROR_UNSUPPORTED_RANGE,
  MBUS_ERROR_UNSUPPORTED_VIF,
  MBUS_ERROR_NEGATIVE_VALUE,
};

// VIF codes

#define MBUS_VIF_DEF_NUM                  71

typedef struct {
  uint8_t code;
  uint32_t base;
  uint8_t size;
  int8_t scalar;
} vif_def_type;

static const vif_def_type vif_defs[MBUS_VIF_DEF_NUM] = {

  // No VIFE
  { MBUS_CODE_ENERGY_WH               , 0x00     , 8,  -3},
  { MBUS_CODE_ENERGY_J                , 0x08     , 8,   0},
  { MBUS_CODE_VOLUME_M3               , 0x10     , 8,  -6},
  { MBUS_CODE_MASS_KG                 , 0x18     , 8,  -3},
  { MBUS_CODE_ON_TIME_S               , 0x20     , 1,   0},
  { MBUS_CODE_ON_TIME_MIN             , 0x21     , 1,   0},
  { MBUS_CODE_ON_TIME_H               , 0x22     , 1,   0},
  { MBUS_CODE_ON_TIME_DAYS            , 0x23     , 1,   0},
  { MBUS_CODE_OPERATING_TIME_S        , 0x24     , 1,   0},
  { MBUS_CODE_OPERATING_TIME_MIN      , 0x25     , 1,   0},
  { MBUS_CODE_OPERATING_TIME_H        , 0x26     , 1,   0},
  { MBUS_CODE_OPERATING_TIME_DAYS     , 0x27     , 1,   0},
  { MBUS_CODE_POWER_W                 , 0x28     , 8,  -3},
  { MBUS_CODE_POWER_J_H               , 0x30     , 8,   0},
  { MBUS_CODE_VOLUME_FLOW_M3_H        , 0x38     , 8,  -6},
  { MBUS_CODE_VOLUME_FLOW_M3_MIN      , 0x40     , 8,  -7},
  { MBUS_CODE_VOLUME_FLOW_M3_S        , 0x48     , 8,  -9},
  { MBUS_CODE_MASS_FLOW_KG_H          , 0x50     , 8,  -3},
  { MBUS_CODE_FLOW_TEMPERATURE_C      , 0x58     , 4,  -3},
  { MBUS_CODE_RETURN_TEMPERATURE_C    , 0x5C     , 4,  -3},
  { MBUS_CODE_TEMPERATURE_DIFF_K      , 0x60     , 4,  -3},
  { MBUS_CODE_EXTERNAL_TEMPERATURE_C  , 0x64     , 4,  -3},
  { MBUS_CODE_PRESSURE_BAR            , 0x68     , 4,  -3},
  //{ MBUS_CODE_TIME_POINT_DATE         , 0x6C     , 1,   0},
  //{ MBUS_CODE_TIME_POINT_DATETIME     , 0x6D     , 1,   0},
  //{ MBUS_CODE_HCA                     , 0x6E     , 1,   0},
  { MBUS_CODE_AVG_DURATION_S          , 0x70     , 1,   0},
  { MBUS_CODE_AVG_DURATION_MIN        , 0x71     , 1,   0},
  { MBUS_CODE_AVG_DURATION_H          , 0x72     , 1,   0},
  { MBUS_CODE_AVG_DURATION_DAYS       , 0x73     , 1,   0},
  { MBUS_CODE_ACTUAL_DURATION_S       , 0x74     , 1,   0},
  { MBUS_CODE_ACTUAL_DURATION_MIN     , 0x75     , 1,   0},
  { MBUS_CODE_ACTUAL_DURATION_H       , 0x76     , 1,   0},
  { MBUS_CODE_ACTUAL_DURATION_DAYS    , 0x77     , 1,   0},
  { MBUS_CODE_FABRICATION_NUMBER      , 0x78     , 1,   0},
  { MBUS_CODE_BUS_ADDRESS             , 0x7A     , 1,   0},

  // VIFE 0xFD
  { MBUS_CODE_CREDIT                  , 0xFD00   ,  4,  -3},
  { MBUS_CODE_DEBIT                   , 0xFD04   ,  4,  -3},
  { MBUS_CODE_ACCESS_NUMBER           , 0xFD08   ,  1,   0},
  //{ MBUS_CODE_MEDIUM                  , 0xFD09   ,  1,   0},
  { MBUS_CODE_MANUFACTURER            , 0xFD0A   ,  1,   0},
  //{ MBUS_CODE_PARAMETER_SET_ID        , 0xFD0B   ,  1,   0},
  { MBUS_CODE_MODEL_VERSION           , 0xFD0C   ,  1,   0},
  { MBUS_CODE_HARDWARE_VERSION        , 0xFD0D   ,  1,   0},
  { MBUS_CODE_FIRMWARE_VERSION        , 0xFD0E   ,  1,   0},
  //{ MBUS_CODE_SOFTWARE_VERSION        , 0xFD0F   ,  1,   0},
  //{ MBUS_CODE_CUSTOMER_LOCATION       , 0xFD10   ,  1,   0},
  { MBUS_CODE_CUSTOMER                , 0xFD11   ,  1,   0},
  //{ MBUS_CODE_ACCESS_CODE_USER        , 0xFD12   ,  1,   0},
  //{ MBUS_CODE_ACCESS_CODE_OPERATOR    , 0xFD13   ,  1,   0},
  //{ MBUS_CODE_ACCESS_CODE_SYSOP       , 0xFD14   ,  1,   0},
  //{ MBUS_CODE_ACCESS_CODE_DEVELOPER   , 0xFD15   ,  1,   0},
  //{ MBUS_CODE_PASSWORD                , 0xFD16   ,  1,   0},
  { MBUS_CODE_ERROR_FLAGS             , 0xFD17   ,  1,   0},
  { MBUS_CODE_ERROR_MASK              , 0xFD18   ,  1,   0},
  { MBUS_CODE_DIGITAL_OUTPUT          , 0xFD1A   ,  1,   0},
  { MBUS_CODE_DIGITAL_INPUT           , 0xFD1B   ,  1,   0},
  { MBUS_CODE_BAUDRATE_BPS            , 0xFD1C   ,  1,   0},
  { MBUS_CODE_RESPONSE_DELAY_TIME     , 0xFD1D   ,  1,   0},
  { MBUS_CODE_RETRY                   , 0xFD1E   ,  1,   0},
  { MBUS_CODE_GENERIC                 , 0xFD3C   ,  1,   0},
  { MBUS_CODE_VOLTS                   , 0xFD40   , 16,  -9},
  { MBUS_CODE_AMPERES                 , 0xFD50   , 16, -12},
  { MBUS_CODE_RESET_COUNTER           , 0xFD60   , 16, -12},
  { MBUS_CODE_CUMULATION_COUNTER      , 0xFD61   , 16, -12},

  // VIFE 0xFB
  { MBUS_CODE_ENERGY_WH               , 0xFB00   , 2,   5},
  { MBUS_CODE_ENERGY_J                , 0xFB08   , 2,   8},
  { MBUS_CODE_VOLUME_M3               , 0xFB10   , 2,   2},
  { MBUS_CODE_MASS_KG                 , 0xFB18   , 2,   5},
  { MBUS_CODE_VOLUME_FT3              , 0xFB21   , 1,  -1},
  { MBUS_CODE_VOLUME_GAL              , 0xFB22   , 2,  -1},
  { MBUS_CODE_VOLUME_FLOW_GAL_M       , 0xFB24   , 1,  -3},
  { MBUS_CODE_VOLUME_FLOW_GAL_M       , 0xFB25   , 1,   0},
  { MBUS_CODE_VOLUME_FLOW_GAL_H       , 0xFB26   , 1,   0},
  { MBUS_CODE_POWER_W                 , 0xFB28   , 2,   5},
  { MBUS_CODE_POWER_J_H               , 0xFB30   , 2,   8},
  { MBUS_CODE_FLOW_TEMPERATURE_F      , 0xFB58   , 4,  -3},
  { MBUS_CODE_RETURN_TEMPERATURE_F    , 0xFB5C   , 4,  -3},
  { MBUS_CODE_TEMPERATURE_DIFF_F      , 0xFB60   , 4,  -3},
  { MBUS_CODE_EXTERNAL_TEMPERATURE_F  , 0xFB64   , 4,  -3},
  { MBUS_CODE_TEMPERATURE_LIMIT_F     , 0xFB70   , 4,  -3},
  { MBUS_CODE_TEMPERATURE_LIMIT_C     , 0xFB74   , 4,  -3},
  { MBUS_CODE_MAX_POWER_W             , 0xFB78   , 8,  -3},

};

class MBUSPayload {

public:

  MBUSPayload(uint8_t size = MBUS_DEFAULT_BUFFER_SIZE);
  ~MBUSPayload();

  void reset(void);
  uint8_t getSize(void);
  uint8_t * getBuffer(void);
  uint8_t copy(uint8_t * buffer);
  uint8_t getError();

  uint8_t addRaw(uint8_t dif, uint32_t vif, uint32_t value);
  uint8_t addField(uint8_t code, int8_t scalar, uint32_t value);
  uint8_t addField(uint8_t code, float value);
  
  uint8_t decode(uint8_t *buffer, uint8_t size, JsonArray& root);
  const char * getCodeUnits(uint8_t code);
  
protected:

  int8_t _findDefinition(uint32_t vif);
  uint32_t _getVIF(uint8_t code, int8_t scalar);

  uint8_t * _buffer;
  uint8_t _maxsize;
  uint8_t _cursor;
  uint8_t _error = MBUS_ERROR_OK;

};

#endif