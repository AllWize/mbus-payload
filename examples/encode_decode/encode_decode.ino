/*

MBUS Payload Encoder / Decoder

Encode-decode Simple example

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
#include "ArduinoJson.h"

#if defined(ARDUINO_ARCH_SAMD)
    #define PC_SERIAL SerialUSB
#else
    #define PC_SERIAL Serial
#endif

void setup() {

    PC_SERIAL.begin(115200);
    while (!PC_SERIAL && millis() < 5000);

    PC_SERIAL.println();
    PC_SERIAL.println("MBUS Payload simple encode-decode example");
    PC_SERIAL.println("=========================================");
    PC_SERIAL.println();

    PC_SERIAL.println("Creating payload buffer...");
    MBUSPayload payload(32);
    
    PC_SERIAL.println("Adding fields...");
    payload.addField(MBUS_CODE::ACCESS_NUMBER, 103);
    payload.addField(MBUS_CODE::EXTERNAL_TEMPERATURE_C, 22.5);
    payload.addField(MBUS_CODE::POWER_W, 1204);
    payload.addField(MBUS_CODE::VOLUME_M3, -3, 57);

    PC_SERIAL.print("Current buffer size: "); PC_SERIAL.println(payload.getSize());
    PC_SERIAL.print("Current buffer contents: ");
    char byte[6];
    for (uint8_t i=0; i<payload.getSize(); i++) {
        snprintf(byte, sizeof(byte), "%02X ", payload.getBuffer()[i]);
        PC_SERIAL.print(byte);
    }
    PC_SERIAL.println();

    PC_SERIAL.println("Decoding...");
    DynamicJsonDocument jsonBuffer(512);
    JsonArray root = jsonBuffer.createNestedArray();  
    uint8_t fields = payload.decode(payload.getBuffer(), payload.getSize(), root); 
    serializeJsonPretty(root, PC_SERIAL);
    PC_SERIAL.println();

    PC_SERIAL.println();
    for (uint8_t i=0; i<fields; i++) {
        float value = root[i]["value_scaled"].as<float>();
        uint8_t code = root[i]["code"].as<int>();
        PC_SERIAL.print("Field "); PC_SERIAL.print(i+1); 
        PC_SERIAL.print(" ("); PC_SERIAL.print((char *) payload.getCodeName(code)); 
        PC_SERIAL.print("): ");
        PC_SERIAL.print(value); PC_SERIAL.print(" "); PC_SERIAL.print((char *) payload.getCodeUnits(code));
        PC_SERIAL.println();
    }

    PC_SERIAL.println();

}

void loop() {
    delay(1);
}
