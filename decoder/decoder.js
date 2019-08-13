/*

MBUS Payload JavaScript Decoder

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

function mbusDecoder(bytes) {
    
    // Regexp C to javascript:
    // replace: { (\w+)\s+,\s(\w+)\s+,\s+(\d+),\s+([-0-9]+)},
    // with: { "name": "$1", "units": "", "base": $2, "size": $3, "scalar": $4},

    var definitions = [

        // No VIFE
        { "name": "energy", "units": "Wh", "base": 0x00 , "size": 8, "scalar": -3},
        { "name": "energy", "units": "J", "base": 0x08, "size": 8, "scalar": 0},
        { "name": "volume", "units": "m3", "base": 0x10, "size": 8, "scalar": -6},
        { "name": "mass", "units": "kg", "base": 0x18, "size": 8, "scalar": -3},
        { "name": "on_time", "units": "s", "base": 0x20, "size": 1, "scalar": 0},
        { "name": "on_time", "units": "min", "base": 0x21, "size": 1, "scalar": 0},
        { "name": "on_time", "units": "h", "base": 0x22, "size": 1, "scalar": 0},
        { "name": "on_time", "units": "days", "base": 0x23, "size": 1, "scalar": 0},
        { "name": "operating_time", "units": "s", "base": 0x24, "size": 1, "scalar": 0},
        { "name": "operating_time", "units": "min", "base": 0x25, "size": 1, "scalar": 0},
        { "name": "operating_time", "units": "h", "base": 0x26, "size": 1, "scalar": 0},
        { "name": "operating_time", "units": "days", "base": 0x27, "size": 1, "scalar": 0},
        { "name": "power", "units": "W", "base": 0x28, "size": 8, "scalar": -3},
        { "name": "power", "units": "J/h", "base": 0x30, "size": 8, "scalar": 0},
        { "name": "volume_flow", "units": "m3/h", "base": 0x38, "size": 8, "scalar": -6},
        { "name": "volume_flow", "units": "m3/min", "base": 0x40, "size": 8, "scalar": -7},
        { "name": "volume_flow", "units": "m3/s", "base": 0x48, "size": 8, "scalar": -9},
        { "name": "mass_flow", "units": "kg/h", "base": 0x50, "size": 8, "scalar": -3},
        { "name": "flow_temperature", "units": "C", "base": 0x58, "size": 4, "scalar": -3},
        { "name": "return_temperature", "units": "C", "base": 0x5C, "size": 4, "scalar": -3},
        { "name": "temperature_difference", "units": "K", "base": 0x60, "size": 4, "scalar": -3},
        { "name": "external_temperature", "units": "C", "base": 0x64, "size": 4, "scalar": -3},
        { "name": "pressure", "units": "bar", "base": 0x68, "size": 4, "scalar": -3},
        //{ "name": "date", "units": "", "base": 0x6C, "size": 1, "scalar": 0},
        //{ "name": "datetime", "units": "", "base": 0x6D, "size": 1, "scalar": 0},
        //{ "name": "hca", "units": "", "base": 0x6E, "size": 1, "scalar": 0},
        { "name": "avg_duration", "units": "s", "base": 0x70, "size": 1, "scalar": 0},
        { "name": "avg_duration", "units": "min", "base": 0x71, "size": 1, "scalar": 0},
        { "name": "avg_duration", "units": "h", "base": 0x72, "size": 1, "scalar": 0},
        { "name": "avg_duration", "units": "days", "base": 0x73, "size": 1, "scalar": 0},
        { "name": "actual_duration", "units": "s", "base": 0x74, "size": 1, "scalar": 0},
        { "name": "actual_duration", "units": "min", "base": 0x75, "size": 1, "scalar": 0},
        { "name": "actual_duration", "units": "h", "base": 0x76, "size": 1, "scalar": 0},
        { "name": "actual_duration", "units": "days", "base": 0x77, "size": 1, "scalar": 0},
        { "name": "fabrication_number", "units": "", "base": 0x78, "size": 1, "scalar": 0},
        { "name": "bus_address", "units": "", "base": 0x7A, "size": 1, "scalar": 0},
      
        // VIFE 0xFD
        { "name": "credit", "units": "", "base": 0xFD00, "size": 4, "scalar": -3},
        { "name": "debit", "units": "", "base": 0xFD04, "size": 4, "scalar": -3},
        { "name": "access_number", "units": "", "base": 0xFD08, "size": 1, "scalar": 0},
        //{ "name": "medium", "units": "", "base": 0xFD09, "size": 1, "scalar": 0},
        { "name": "manufacturer", "units": "", "base": 0xFD0A, "size": 1, "scalar": 0},
        //{ "name": "parameter_set_id", "units": "", "base": 0xFD0B, "size": 1, "scalar": 0},
        { "name": "model_version", "units": "", "base": 0xFD0C, "size": 1, "scalar": 0},
        { "name": "hardware_version", "units": "", "base": 0xFD0D, "size": 1, "scalar": 0},
        { "name": "firmware_version", "units": "", "base": 0xFD0E, "size": 1, "scalar": 0},
        //{ "name": "software_version", "units": "", "base": 0xFD0F, "size": 1, "scalar": 0},
        //{ "name": "customer_location", "units": "", "base": 0xFD10, "size": 1, "scalar": 0},
        { "name": "customer", "units": "", "base": 0xFD11, "size": 1, "scalar": 0},
        //{ "name": "access_code_user", "units": "", "base": 0xFD12, "size": 1, "scalar": 0},
        //{ "name": "access_code_operator", "units": "", "base": 0xFD13, "size": 1, "scalar": 0},
        //{ "name": "access_code_sysop", "units": "", "base": 0xFD14, "size": 1, "scalar": 0},
        //{ "name": "access_code_developer", "units": "", "base": 0xFD15, "size": 1, "scalar": 0},
        //{ "name": "password", "units": "", "base": 0xFD16, "size": 1, "scalar": 0},
        { "name": "error_flags", "units": "", "base": 0xFD17, "size": 1, "scalar": 0},
        { "name": "error_mask", "units": "", "base": 0xFD18, "size": 1, "scalar": 0},
        { "name": "digital_output", "units": "", "base": 0xFD1A, "size": 1, "scalar": 0},
        { "name": "digital_input", "units": "", "base": 0xFD1B, "size": 1, "scalar": 0},
        { "name": "baudrate", "units": "bps", "base": 0xFD1C, "size": 1, "scalar": 0},
        { "name": "response_delay_time", "units": "", "base": 0xFD1D, "size": 1, "scalar": 0},
        { "name": "retry", "units": "", "base": 0xFD1E, "size": 1, "scalar": 0},
        { "name": "generic", "units": "", "base": 0xFD3C, "size": 1, "scalar": 0},
        { "name": "volts", "units": "V", "base": 0xFD40, "size": 16, "scalar": -9},
        { "name": "amperes", "units": "A", "base": 0xFD50, "size": 16, "scalar": -12},
        { "name": "reset_counter", "units": "", "base": 0xFD60, "size": 16, "scalar": -12},
        { "name": "cumulation_counter", "units": "", "base": 0xFD61, "size": 16, "scalar": -12},
      
        // VIFE 0xFB
        { "name": "energy", "units": "Wh", "base": 0xFB00, "size": 2, "scalar": 5},
        { "name": "energy", "units": "J", "base": 0xFB08, "size": 2, "scalar": 8},
        { "name": "volume", "units": "m3", "base": 0xFB10, "size": 2, "scalar": 2},
        { "name": "mass", "units": "kg", "base": 0xFB18, "size": 2, "scalar": 5},
        { "name": "volume", "units": "ft3", "base": 0xFB21, "size": 1, "scalar": -1},
        { "name": "volume", "units": "gal", "base": 0xFB22, "size": 2, "scalar": -1},
        { "name": "volume_flow", "units": "gal/min", "base": 0xFB24, "size": 1, "scalar": -3},
        { "name": "volume_flow", "units": "gal/min", "base": 0xFB25, "size": 1, "scalar": 0},
        { "name": "volume_flow", "units": "gal/h", "base": 0xFB26, "size": 1, "scalar": 0},
        { "name": "power", "units": "W", "base": 0xFB28, "size": 2, "scalar": 5},
        { "name": "power", "units": "J/h", "base": 0xFB30, "size": 2, "scalar": 8},
        { "name": "flow_temperature", "units": "F", "base": 0xFB58, "size": 4, "scalar": -3},
        { "name": "return_temperature", "units": "F", "base": 0xFB5C, "size": 4, "scalar": -3},
        { "name": "temperature_difference", "units": "F", "base": 0xFB60, "size": 4, "scalar": -3},
        { "name": "external_temperature", "units": "F", "base": 0xFB64, "size": 4, "scalar": -3},
        { "name": "temperature_limit", "units": "F", "base": 0xFB70, "size": 4, "scalar": -3},
        { "name": "temperature_limit", "units": "C", "base": 0xFB74, "size": 4, "scalar": -3},
        { "name": "max_power", "units": "W", "base": 0xFB78, "size": 8, "scalar": -3},
      
    ];
      
    function bin2dec(stream) {
        var value = 0;
        for (var i = 0; i < stream.length; i++) {
            var byte = stream[stream.length - i - 1];
            if (byte > 0xFF) {
                throw "Byte value overflow!";
            }
            value = (value << 8) | byte;
        }
        return value;
    }

    function bcd2dec(stream) {
        var value = 0;
        for (var i = 0; i < stream.length; i++) {
            var byte = stream[stream.length - i - 1];
            if (byte > 0xFF) {
                throw "Byte value overflow!";
            }
            value = (value * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
        }
        return value;
    }

    var fields = [];
    var index = 0;
    while (index < bytes.length) {

        // Decode DIF
        var dif = bytes[index++];
        var bcd = ((dif & 0x08) == 0x08);
        var len = (dif & 0x07);
        if ((len < 1) || (4 < len)) {
            throw "Unsupported coding: " + (dif & 0x0F);
        }
    
        // Get VIF(E)
        var vif = 0;
        do {
            if (index == bytes.length) {
                throw "Buffer overflow";
            }
            vif = (vif << 8) | bytes[index++];
        } while ((vif & 0x80) == 0x80);

        // Find definition
        var def = -1;
        for (var i = 0; i < definitions.length; i++) {
            if ((definitions[i].base <= vif) && (vif < (definitions[i].base + definitions[i].size))) {
                def = i;
                break;
            }
        }
        if (def < 0) {
            throw "Unsupported VIF: " + vif;
        }

        // Check buffer overflow
        if (index + len > bytes.length) {
            throw "Buffer overflow";
        }

        // read value
        var value = bcd
            ? bcd2dec(bytes.slice(index, index+len))
            : bin2dec(bytes.slice(index, index+len));
        index += len;

        // scaled value
        var scalar = definitions[def].scalar + vif - definitions[def].base;
        var scaled = value * Math.pow(10, scalar);

        // Add field
        fields.push({
            "index": fields.length + 1,
            "vif": vif,
            "name": definitions[def].name,
            "units":  definitions[def].units,
            "value": scaled
        });

    }

    return fields;

}

// To use with TTN
function Decoder(bytes, fPort) {
    return {"fields": mbusDecoder(bytes)};
}
  
// To use with NodeRED
// Assuming msg.payload contains the LPP-encoded byte array
/*
msg.fields = mbusDecoder(msg.payload);
return msg;
*/