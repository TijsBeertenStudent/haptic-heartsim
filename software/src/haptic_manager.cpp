// ============================================================================
// haptic_manager.cpp — Implementation of the 4-module haptic driver
// ----------------------------------------------------------------------------
// IMPLEMENTATION NOTES
//   - On every call we first switch the TCA9548A to the requested channel,
//     then talk to the DRV2605L over I2C. The mux switching adds only a
//     few hundred microseconds — negligible compared to the typical
//     50-100 ms pulse duration.
//   - Each pulse() call sequence is: take chip out of standby → write
//     PWM duty cycle on HAPTIC_PWM_PIN → delay(duration_ms) → set PWM to 0
//     → put chip back into standby (to conserve current).
//   - analogWrite() on the ESP32-S3 defaults to 8-bit resolution under
//     the Arduino core, which matches the 0..255 intensity range we use
//     throughout the codebase.
//   - Each module has an independent "enabled" flag controlled by the web
//     UI; a disabled module silently skips pulse() calls.
//   - The master intensity multiplier provides a single slider that
//     scales all haptic pulses uniformly.
// ============================================================================
#include "haptic_manager.h"
#include "tca9548a.h"

HapticManager::HapticManager()
    : _master_intensity(200) {
    for (uint8_t i = 0; i < NUM_MODULES; i++) {
        _module_enabled[i] = true;
    }
}

bool HapticManager::begin() {
    // I2C setup — Wire.begin() gebruikt default SDA/SCL (A4/A5 op Nano ESP32)
    Wire.begin();
    Wire.setClock(I2C_FREQ);

    pinMode(HAPTIC_PWM_PIN, OUTPUT);
    analogWrite(HAPTIC_PWM_PIN, 0);

    Serial.println("[HAPTIC] Initialiseren...");

    // Check of de mux er is
    if (!TCA9548A::isPresent()) {
        Serial.println("[HAPTIC] FOUT: TCA9548A niet gevonden op 0x70!");
        return false;
    }
    Serial.println("[HAPTIC] OK: TCA9548A gevonden");

    // Initialiseer elke DRV2605L
    bool all_ok = true;
    for (uint8_t m = 0; m < NUM_MODULES; m++) {
        TCA9548A::selectChannel(m);
        delay(5);

        // Check of DRV antwoordt op dit kanaal
        Wire.beginTransmission(DRV2605_ADDR);
        if (Wire.endTransmission() != 0) {
            Serial.printf("[HAPTIC] FOUT: geen DRV2605L op kanaal %u\n", m);
            all_ok = false;
            continue;
        }

        _initializeDRV(m);
        Serial.printf("[HAPTIC] OK: module %u geinitialiseerd\n", m);
    }

    return all_ok;
}

void HapticManager::_initializeDRV(uint8_t module) {
    // Deze sequentie komt overeen met initializeDRV2605() in jullie originele
    // motor_code.ino — uitgelegd:
    //
    // Reg 0x01 (MODE):
    //   0x00 = internal trigger
    //   0x03 = PWM input mode
    //   0x43 = standby + PWM input mode
    //
    // Reg 0x1D (Control3):
    //   0xA8 = ERM library + N_PWM_ANALOG=PWM input (TS2200 setting)
    //
    // Reg 0x03 (LIBRARY_SELECTION):
    //   0x01 = Library A (ERM coin motor)
    //
    // Reg 0x16 (RATED_VOLTAGE): 0xFF = max
    // Reg 0x17 (OD_CLAMP):       0xFF = max overdrive

    _writeDRVRegister(module, 0x01, 0x00);  // Wake up, internal trigger
    _writeDRVRegister(module, 0x1D, 0xA8);  // ERM, PWM input enabled
    _writeDRVRegister(module, 0x03, 0x01);  // Library 1 (coin motor)
    _writeDRVRegister(module, 0x16, 0xFF);  // Max rated voltage
    _writeDRVRegister(module, 0x17, 0xFF);  // Max overdrive
    _writeDRVRegister(module, 0x01, 0x43);  // Standby + PWM mode
}

void HapticManager::_writeDRVRegister(uint8_t module, uint8_t reg, uint8_t value) {
    TCA9548A::selectChannel(module);
    Wire.beginTransmission(DRV2605_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void HapticManager::_setStandby(uint8_t module, bool standby) {
    // Reg 0x01: 0x43 = standby aan + PWM mode, 0x03 = standby uit + PWM mode
    _writeDRVRegister(module, 0x01, standby ? 0x43 : 0x03);
}

void HapticManager::pulse(uint8_t module, float intensity, uint16_t duration_ms) {
    if (module >= NUM_MODULES) return;
    if (!_module_enabled[module]) return;

    // Pas master intensity toe
    float scaled = intensity * (_master_intensity / 255.0f);
    if (scaled < 0.0f) scaled = 0.0f;
    if (scaled > 1.0f) scaled = 1.0f;

    // Conversie naar PWM: jullie originele code gebruikte een minimum
    // van 140 voor "voelbaarheid" — we behouden dat.
    constexpr int PWM_MIN = 140;
    constexpr int PWM_MAX = 255;
    int pwm = (scaled <= 0.0f)
        ? 0
        : (int)(scaled * (PWM_MAX - PWM_MIN)) + PWM_MIN;

    _setStandby(module, false);             // Wake up
    analogWrite(HAPTIC_PWM_PIN, pwm);
    delay(duration_ms);
    analogWrite(HAPTIC_PWM_PIN, 0);
    _setStandby(module, true);              // Spaar stroom
}

void HapticManager::playLibraryEffect(uint8_t module, uint8_t effect_id) {
    if (module >= NUM_MODULES) return;
    if (!_module_enabled[module]) return;
    if (effect_id < 1 || effect_id > 123) return;

    // Schakel terug naar internal trigger mode voor library playback
    _writeDRVRegister(module, 0x01, 0x00);   // mode = internal trigger
    _writeDRVRegister(module, 0x04, effect_id); // waveform slot 0
    _writeDRVRegister(module, 0x05, 0x00);   // einde sequence
    _writeDRVRegister(module, 0x0C, 0x01);   // GO!
}

void HapticManager::stop(uint8_t module) {
    if (module >= NUM_MODULES) return;
    analogWrite(HAPTIC_PWM_PIN, 0);
    _setStandby(module, true);
}

void HapticManager::setModuleEnabled(uint8_t module, bool enabled) {
    if (module < NUM_MODULES) {
        _module_enabled[module] = enabled;
        if (!enabled) stop(module);
    }
}

bool HapticManager::isModuleEnabled(uint8_t module) const {
    return (module < NUM_MODULES) ? _module_enabled[module] : false;
}
