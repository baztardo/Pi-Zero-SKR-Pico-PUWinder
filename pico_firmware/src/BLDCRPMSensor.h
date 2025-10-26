/**
 * BLDCRPMSensor.h
 * BLDC Motor RPM sensor class using speed pulse output
 * 8 poles = 4 pole pairs
 * 3 Phase BLDC Motor
 * 6 Hal Phase sequeces 
 * Phase    A     B     C
 * 1       ha     hb     hc
 * 2       hc     ha     hb
 * 3       hb     hc     ha
 * 4       ha     hc     hc
 * 5       hb     ha     hc
 * 6       hc     hb     ha
 * 
 * 
 */

 #ifndef BLDC_RPM_SENSOR_H
 #define BLDC_RPM_SENSOR_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 class BLDCRPMSensor {
 public:
     // Constructor
     BLDCRPMSensor(float pulsesPerRev = 18.0f, uint32_t timeoutUs = 500000);
     
     // Interrupt handler - call from your hardware interrupt
     void onPulseReceived(uint64_t currentTimeMicros);
     
     // Get RPM - instantaneous (fast but noisy)
     float getRPM_Instant() const;
     
     // Get RPM - averaged over time window (stable, recommended)
     float getRPM_Averaged(uint64_t currentTimeMillis);
     
     // Utility functions
     bool isMotorRunning(uint64_t currentTimeMicros) const;
     uint32_t getTotalPulses() const;
     void resetPulseCounter();
     bool hasNewPulse();
     uint64_t getPulsePeriod() const;
     
    // Configuration
    void setPulsesPerRev(float pulses);
    void setTimeout(uint32_t timeoutUs);
    void setUpdateInterval(uint32_t intervalMs);
    void setMinPulsePeriod(uint32_t minPeriodUs);
     
 private:
    // Configuration
    float pulsesPerRevolution;
    uint32_t timeoutMicros;
    uint32_t updateIntervalMs;
    uint32_t minPulsePeriodUs;
     
     // Pulse timing
     volatile uint64_t lastPulseTime;
     volatile uint64_t pulsePeriod;
     volatile uint32_t pulseCount;
     volatile bool newPulseFlag;
     
     // Averaged calculation state
     uint32_t lastPulseCount;
     uint64_t lastUpdateTime;
     float lastAveragedRPM;
 };
 
 #endif // BLDC_RPM_SENSOR_H