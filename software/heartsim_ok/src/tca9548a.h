// ============================================================================
// tca9548a.h — Lightweight wrapper around the TCA9548A I2C multiplexer
// ----------------------------------------------------------------------------
// WHY WE NEED A MULTIPLEXER
//   All four DRV2605L haptic drivers share the same I2C address (0x5A,
//   not user-configurable on this breakout). Without a mux they would
//   collide on the same bus. The TCA9548A presents a single I2C address
//   (0x70) and switches the SDA/SCL lines internally to one of its eight
//   "downstream" channels, so each driver sees its own private bus.
//
// PROTOCOL
//   - To select a channel we send one byte to address 0x70 containing
//     a bitmask: bit N enables channel N.
//   - We always set exactly one bit at a time (bitmask = 1 << channel)
//     so a single driver receives our commands.
//   - This wrapper is implemented as inline functions so there is no
//     ".cpp" file and zero call overhead — selectChannel() compiles down
//     to three direct Wire library calls.
// ============================================================================
#pragma once
#include <Wire.h>
#include "config.h"

namespace TCA9548A {

    /**
     * Selecteer één kanaal op de multiplexer.
     * @param channel 0..7 — bit-bestand bepaalt welke kanalen ENABLED zijn.
     *
     * Schrijfprotocol: 1 byte naar TCA9548A_ADDR met de bitmask van actieve
     * kanalen. We selecteren altijd precies één kanaal tegelijk.
     */
    inline void selectChannel(uint8_t channel) {
        Wire.beginTransmission(TCA9548A_ADDR);
        Wire.write(1 << channel);
        Wire.endTransmission();
    }

    /**
     * Test of de multiplexer aanwezig is op de I2C-bus.
     * @return true als acknowledged op TCA9548A_ADDR.
     */
    inline bool isPresent() {
        Wire.beginTransmission(TCA9548A_ADDR);
        return (Wire.endTransmission() == 0);
    }

    /**
     * Scan elk van de 4 gebruikte kanalen en print welke I2C-devices erop
     * antwoorden. Handig om setup-fouten op te sporen.
     */
    inline void debugScanAllChannels() {
        Serial.println("[TCA] Scanning all 4 channels...");
        for (uint8_t ch = 0; ch < NUM_MODULES; ch++) {
            selectChannel(ch);
            delay(5);
            Serial.printf("  Channel %u:", ch);
            bool found = false;
            for (uint8_t addr = 0x08; addr < 0x78; addr++) {
                if (addr == TCA9548A_ADDR) continue; // sla mux zelf over
                Wire.beginTransmission(addr);
                if (Wire.endTransmission() == 0) {
                    Serial.printf(" 0x%02X", addr);
                    found = true;
                }
            }
            if (!found) Serial.print(" (geen)");
            Serial.println();
        }
    }
}
