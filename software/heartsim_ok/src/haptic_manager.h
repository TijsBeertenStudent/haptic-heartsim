// ============================================================================
// haptic_manager.h — Driver for the 4 haptic modules
// ----------------------------------------------------------------------------
// HARDWARE PER MODULE
//   - 1 × DRV2605L haptic driver IC on TCA9548A channel N (N = 0..3)
//   - 1..3 coin vibration motors wired directly between OUT+ and OUT-
//     of the DRV2605L. The chip contains an internal H-bridge plus
//     flyback protection for inductive loads, so as long as we stay
//     below the rated ≈250 mA continuous output (= 2-3 coin motors in
//     parallel) no external MOSFET or diode is required.
//
// HOW ONE PWM PIN CONTROLS FOUR DRIVERS
//   HAPTIC_PWM_PIN (GPIO 13) drives the IN/PWM input of all four DRV2605L
//   chips in parallel. Only the driver whose channel is currently selected
//   on the TCA9548A receives our I2C control bytes. By switching the mux
//   channel before each pulse() call, we can address a single module while
//   the PWM line itself is shared.
//
// DRV2605L MODE
//   We program each chip into "PWM input mode" (register 0x01 = 0x43 for
//   standby, 0x03 for active). In this mode the chip multiplies its
//   internal real-time-playback (RTP) register by the duty cycle of the
//   PWM pin, giving fine-grained amplitude control without I2C latency.
// ============================================================================
#pragma once
#include <Arduino.h>
#include "config.h"

class HapticManager {
public:
    HapticManager();

    /**
     * Initialiseer de I2C-bus, multiplexer, alle 4 DRV2605L's en de PWM-pin.
     * @return true als alle 4 DRV's geantwoord hebben.
     */
    bool begin();

    /**
     * Speel een korte trilling op één module (zoals jullie pulse()-functie).
     * @param module      0..3 (POINT_AORTIC..POINT_MITRAL)
     * @param intensity   0.0 .. 1.0
     * @param duration_ms duur in milliseconden
     *
     * BLOCKING — gebruik dit alleen vanuit een dedicated taak of vanuit de
     * cycle engine die in een eigen task draait.
     */
    void pulse(uint8_t module, float intensity, uint16_t duration_ms);

    /**
     * Niet-blocking variant: programmeert een effect-id uit de DRV2605
     * library (1..123) op de gekozen module en start het. De DRV speelt
     * het zelf af, deze functie keert direct terug.
     *
     * Nuttig voor lage-frequentie "voelbare" componenten (S3, S4).
     */
    void playLibraryEffect(uint8_t module, uint8_t effect_id);

    /** Stop een lopend effect op een module. */
    void stop(uint8_t module);

    /** Master intensiteit (0..255) — vermenigvuldigt alle pulses. */
    void setMasterIntensity(uint8_t intensity) { _master_intensity = intensity; }
    uint8_t getMasterIntensity() const { return _master_intensity; }

    /** Enable/disable per module (master switch vanuit web UI). */
    void setModuleEnabled(uint8_t module, bool enabled);
    bool isModuleEnabled(uint8_t module) const;

private:
    uint8_t _master_intensity;          // 0..255
    bool    _module_enabled[NUM_MODULES];

    void _initializeDRV(uint8_t module);
    void _writeDRVRegister(uint8_t module, uint8_t reg, uint8_t value);
    void _setStandby(uint8_t module, bool standby);
};
