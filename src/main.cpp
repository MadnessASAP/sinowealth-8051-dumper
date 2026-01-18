/*
   https://github.com/gashtaan/sinowealth-8051-dumper

   Copyright (C) 2023, Michal Kovacik

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include "rpc.h"

// Arduino setup function
void setup() {
    // Initialize serial using Arduino's Serial object for SimpleRPC compatibility
    Serial.begin(115200);
    Serial.println(F("\r\nSinoWealth 8051-based MCU flash dumper (RPC mode)"));
    Serial.println(F("Ready for commands"));

    rpc_init();
}

// Arduino loop function - handles RPC calls
void loop() {
    rpc_loop();
}

// Main function for non-Arduino builds
int main() {
    init();  // Arduino initialization
    setup();
    while (true) {
        loop();
    }
    return 0;
}
