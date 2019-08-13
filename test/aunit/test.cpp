/*

MBUS Payload Encoder / Decoder

Unit Tests

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

#include <Arduino.h>
#include "MBUSPayload.h"
#include <AUnit.h>

using namespace aunit;

#define MBUS_PAYLOAD_TEST_VERBOSE 1

#if defined(ARDUINO_ARCH_SAMD)
    #define PC_SERIAL SerialUSB
#else
    #define PC_SERIAL Serial
#endif

// -----------------------------------------------------------------------------
// Wrapper class
// -----------------------------------------------------------------------------

class MBUSPayloadWrap: public MBUSPayload {
    
    public:
        
        MBUSPayloadWrap() : MBUSPayload() {}
        MBUSPayloadWrap(uint8_t size) : MBUSPayload(size) {}
        int8_t findDefinition(uint32_t vif) { return _findDefinition(vif); }
        uint32_t getVIF(uint8_t code, int8_t scalar) { return _getVIF(code, scalar); }

};

// -----------------------------------------------------------------------------
// Helper methods
// -----------------------------------------------------------------------------

class EncoderTest: public TestOnce {

    protected:

        virtual void setup() override {
            mbuspayload = new MBUSPayloadWrap();
            mbuspayload->reset();
        }

        virtual void teardown() override {
            delete mbuspayload;
        }

        virtual void compare(unsigned char depth, uint8_t * expected) {
            
            uint8_t * actual = mbuspayload->getBuffer();
            
            #if MBUS_PAYLOAD_TEST_VERBOSE
                PC_SERIAL.println();
                char buff[6];
                PC_SERIAL.print("Expected: ");
                for (unsigned char i=0; i<depth; i++) {
                    snprintf(buff, sizeof(buff), "%02X ", expected[i]);
                    PC_SERIAL.print(buff);
                }
                PC_SERIAL.println();
                PC_SERIAL.print("Actual  : ");
                for (unsigned char i=0; i<mbuspayload->getSize(); i++) {
                    snprintf(buff, sizeof(buff), "%02X ", actual[i]);
                    PC_SERIAL.print(buff);
                }
                PC_SERIAL.println();
            #endif

            assertEqual(depth, mbuspayload->getSize());
            for (unsigned char i=0; i<depth; i++) {
                assertEqual(expected[i], actual[i]);
            }

        }

        MBUSPayloadWrap * mbuspayload;

};

class DecoderTest: public TestOnce {

    protected:

        virtual void setup() override {
            mbuspayload = new MBUSPayloadWrap(10);
            mbuspayload->reset();
        }

        virtual void teardown() override {
            delete mbuspayload;
        }

        virtual void compare(uint8_t * buffer, unsigned char len, uint8_t fields, uint8_t code = 0xFF, int8_t scalar = 0, uint32_t value = 0) {
            
            DynamicJsonDocument jsonBuffer(256);
            JsonArray root = jsonBuffer.createNestedArray();    
            assertEqual(fields, mbuspayload->decode(buffer, len, root));
            assertEqual(fields, (uint8_t) root.size());

            #if MBUS_PAYLOAD_TEST_VERBOSE
                PC_SERIAL.println();
                serializeJsonPretty(root, PC_SERIAL);
                PC_SERIAL.println();
            #endif

            if (code != 0xFF) {
                assertEqual(code, (uint8_t) root[0]["code"]);
                assertEqual(scalar, (int8_t) root[0]["scalar"]);
                assertEqual(value, (uint32_t) root[0]["value_raw"]);
            }

        }

        MBUSPayloadWrap * mbuspayload;
        

};

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

testF(EncoderTest, Empty) {
    mbuspayload->reset();
    assertEqual(0, mbuspayload->getSize());
}

testF(EncoderTest, Unsupported_Coding) {
    mbuspayload->addRaw(0x0F, 0x06, 14);
    assertEqual(MBUS_ERROR_UNSUPPORTED_CODING, mbuspayload->getError());
    assertEqual(MBUS_ERROR_OK, mbuspayload->getError());
}

testF(EncoderTest, Add_Raw_8bit) {
    uint8_t expected[] = { 0x01, 0x06, 0x0E};
    mbuspayload->addRaw(MBUS_CODING_8BIT, 0x06, 14);
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Raw_16bit) {
    uint8_t expected[] = { 0x02, 0x06, 0x0E, 0x00};
    mbuspayload->addRaw(MBUS_CODING_16BIT, 0x06, 14);
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Raw_32bit) {
    uint8_t expected[] = { 0x04, 0x06, 0x0E, 0x00, 0x00, 0x00};
    mbuspayload->addRaw(MBUS_CODING_32BIT, 0x06, 14);
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Raw_2bcd) {
    uint8_t expected[] = { 0x09, 0x06, 0x14};
    mbuspayload->addRaw(MBUS_CODING_2BCD, 0x06, 14);
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Raw_8bcd) {
    uint8_t expected[] = { 0x0C, 0x13, 0x13, 0x20, 0x00, 0x00};
    mbuspayload->addRaw(MBUS_CODING_8BCD, 0x13, 2013);
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Raw_VIFE) {
    uint8_t expected[] = { 0x01, 0xFB, 0x8C, 0x74, 0x0E};
    mbuspayload->addRaw(MBUS_CODING_8BIT, 0xFB8C74, 14);
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Find_Definition) {
    assertEqual((int8_t) 0, mbuspayload->findDefinition(0x03));
}

testF(EncoderTest, Get_VIF) {
    assertEqual((uint32_t) 0xFF  , mbuspayload->getVIF(MBUS_CODE_ENERGY_WH, -4));
    assertEqual((uint32_t) 0x00  , mbuspayload->getVIF(MBUS_CODE_ENERGY_WH, -3));
    assertEqual((uint32_t) 0x03  , mbuspayload->getVIF(MBUS_CODE_ENERGY_WH,  0));
    assertEqual((uint32_t) 0x06  , mbuspayload->getVIF(MBUS_CODE_ENERGY_WH,  3));
    assertEqual((uint32_t) 0x07  , mbuspayload->getVIF(MBUS_CODE_ENERGY_WH,  4));
    assertEqual((uint32_t) 0xFB00, mbuspayload->getVIF(MBUS_CODE_ENERGY_WH,  5));
    assertEqual((uint32_t) 0xFB01, mbuspayload->getVIF(MBUS_CODE_ENERGY_WH,  6));
    assertEqual((uint32_t) 0xFF  , mbuspayload->getVIF(MBUS_CODE_ENERGY_WH,  7));
}

testF(EncoderTest, Add_Field_1a) {
    uint8_t expected[] = { 0x02, 0x06, 0x78, 0x05 };
    mbuspayload->addField(MBUS_CODE_ENERGY_WH, 3, 1400); // 1400 kWh
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_1b) {
    uint8_t expected[] = { 0x01, 0x07, 0x8C };
    mbuspayload->addField(MBUS_CODE_ENERGY_WH, 4, 140); // 1400 kWh
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_2) {
    uint8_t expected[] = { 0x01, 0xFB, 0x01, 0xC8 };
    mbuspayload->addField(MBUS_CODE_ENERGY_WH, 6, 200); // 200 MWh
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_3) {
    uint8_t expected[] = { 0x01, 0x0D, 0x24 };
    mbuspayload->addField(MBUS_CODE_ENERGY_J, 5, 36); // 3.6 MJ
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_4) {
    uint8_t expected[] = { 0x01, 0x13, 0x39 };
    mbuspayload->addField(MBUS_CODE_VOLUME_M3, -3, 57); // 57 l
    compare(sizeof(expected), expected);
}

testF(EncoderTest, MultiField) {
    uint8_t expected[] = { 0x01, 0x13, 0x39, 0x01, 0x0D, 0x24 };
    mbuspayload->addField(MBUS_CODE_VOLUME_M3, -3, 57); // 57 l
    mbuspayload->addField(MBUS_CODE_ENERGY_J, 5, 36); // 3.6 MJ
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_Compact_1) {
    uint8_t expected[] = { 0x01, 0x13, 0x39 };
    mbuspayload->addField(MBUS_CODE_VOLUME_M3, 0.057); // 57 l
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_Compact_2) {
    uint8_t expected[] = { 0x01, 0x0D, 0x24 };
    mbuspayload->addField(MBUS_CODE_ENERGY_J, 36e5); // 3.6 MJ
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_Compact_3) {
    uint8_t expected[] = { 0x02, 0x2A, 0x06, 0x05 };
    mbuspayload->addField(MBUS_CODE_POWER_W, 128.6); // 128.6 W
    compare(sizeof(expected), expected);
}

testF(EncoderTest, Add_Field_Compact_Zero) {
    uint8_t expected[] = { 0x01, 0x2B, 0x00 };
    mbuspayload->addField(MBUS_CODE_POWER_W, 0); // 0 W
    compare(sizeof(expected), expected);
}

// -----------------------------------------------------------------------------
testF(DecoderTest, Number_1) {
    uint8_t buffer[] = { 0x01, 0xFB, 0x01, 0xC8};
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_ENERGY_WH, 6, 200);
}

testF(DecoderTest, Number_2) {
    uint8_t buffer[] = { 0x01, 0x13, 0x39 };
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_VOLUME_M3, -3, 57);
}

testF(DecoderTest, Number_3) {
    uint8_t buffer[] = { 0x02, 0x06, 0x78, 0x05 };
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_ENERGY_WH, 3, 1400);
}

testF(DecoderTest, Number_4) {
    uint8_t buffer[] = { 0x01, 0x07, 0x8C };
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_ENERGY_WH, 4, 140);
}

testF(DecoderTest, Number_5) {
    uint8_t buffer[] = { 0x01, 0xFB, 0x01, 0xC8 };
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_ENERGY_WH, 6, 200);
}

testF(DecoderTest, Number_6) {
    uint8_t buffer[] = { 0x01, 0x2B, 0x00 };
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_POWER_W, 0, 0);
}

testF(DecoderTest, MultiField) {
    uint8_t buffer[] = { 0x01, 0x13, 0x39, 0x01, 0x0D, 0x24 };
    compare(buffer, sizeof(buffer), 2, MBUS_CODE_VOLUME_M3, -3, 57);
}

testF(DecoderTest, Decode_2bcd) {
    uint8_t buffer[] = { 0x09, 0x06, 0x14};
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_ENERGY_WH, 3, 14);
}

testF(DecoderTest, Decode_8bcd) {
    uint8_t buffer[] = { 0x0C, 0x13, 0x13, 0x20, 0x00, 0x00};
    compare(buffer, sizeof(buffer), 1, MBUS_CODE_VOLUME_M3, -3, 2013);
}
// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

void setup() {

    PC_SERIAL.begin(115200);
    while (!PC_SERIAL && millis() < 5000);

    Printer::setPrinter(&PC_SERIAL);
    //TestRunner::setVerbosity(Verbosity::kAll);

}

void loop() {
    TestRunner::run();
    delay(1);
}
