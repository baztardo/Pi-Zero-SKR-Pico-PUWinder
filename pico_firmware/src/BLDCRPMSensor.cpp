/**
 * BLDCRPMSensor.cpp
 * BLDC Motor RPM sensor implementation
 */

 #include "BLDCRPMSensor.h"

 // ============================================================================
 // CONSTRUCTOR
 // ============================================================================
 BLDCRPMSensor::BLDCRPMSensor(float pulsesPerRev, uint32_t timeoutUs)
     : pulsesPerRevolution(pulsesPerRev)
     , timeoutMicros(timeoutUs)
     , updateIntervalMs(100)
     , minPulsePeriodUs(500)  // NEW: Default 500us filter
     , lastPulseTime(0)
     , pulsePeriod(0)
     , pulseCount(0)
     , newPulseFlag(false)
     , lastPulseCount(0)
     , lastUpdateTime(0)
     , lastAveragedRPM(0.0f)
 {
 }
 
 // ============================================================================
 // INTERRUPT HANDLER - WITH PWM CARRIER FILTER
 // ============================================================================
 void BLDCRPMSensor::onPulseReceived(uint64_t currentTimeMicros) {
     uint64_t timeSinceLastPulse = currentTimeMicros - lastPulseTime;
     
     // **FIX: Filter out PWM carrier noise**
     // Ignore pulses that are too fast (< 500us by default)
     // This filters the 49MHz PWM carrier and only counts actual speed pulses
     if (timeSinceLastPulse < minPulsePeriodUs) {
         return;  // Ignore this pulse - it's PWM carrier noise
     }
     
     // Valid pulse - record it
     pulsePeriod = timeSinceLastPulse;
     lastPulseTime = currentTimeMicros;
     pulseCount++;
     newPulseFlag = true;
 }
 
 // ============================================================================
 // RPM CALCULATION - INSTANTANEOUS
 // ============================================================================
 float BLDCRPMSensor::getRPM_Instant() const {
     // Check for timeout (motor stopped)
     if (pulsePeriod == 0 || pulsePeriod > timeoutMicros) {
         return 0.0f;
     }
     
     // Calculate frequency from period (microseconds to Hz)
     float frequency = 1000000.0f / static_cast<float>(pulsePeriod);
     
     // Convert to RPM
     float rpm = (frequency * 60.0f) / pulsesPerRevolution;
     
     return rpm;
 }
 
 // ============================================================================
 // RPM CALCULATION - AVERAGED (RECOMMENDED)
 // ============================================================================
 float BLDCRPMSensor::getRPM_Averaged(uint64_t currentTimeMillis) {
     // Check if it's time to update
     if (currentTimeMillis - lastUpdateTime >= updateIntervalMs) {
         uint32_t pulsesReceived = pulseCount - lastPulseCount;
         uint64_t timeElapsed = currentTimeMillis - lastUpdateTime;
         
         if (timeElapsed > 0 && pulsesReceived > 0) {
             // Calculate frequency (pulses per second)
             float frequency = (pulsesReceived * 1000.0f) / static_cast<float>(timeElapsed);
             
             // Convert to RPM
             lastAveragedRPM = (frequency * 60.0f) / pulsesPerRevolution;
         } else {
             // No pulses received - motor stopped
             lastAveragedRPM = 0.0f;
         }
         
         lastPulseCount = pulseCount;
         lastUpdateTime = currentTimeMillis;
     }
     
     return lastAveragedRPM;
 }
 
 // ============================================================================
 // UTILITY FUNCTIONS
 // ============================================================================
 bool BLDCRPMSensor::isMotorRunning(uint64_t currentTimeMicros) const {
     return (currentTimeMicros - lastPulseTime) < timeoutMicros;
 }
 
 uint32_t BLDCRPMSensor::getTotalPulses() const {
     return pulseCount;
 }
 
 void BLDCRPMSensor::resetPulseCounter() {
     pulseCount = 0;
     lastPulseCount = 0;
 }
 
 bool BLDCRPMSensor::hasNewPulse() {
     if (newPulseFlag) {
         newPulseFlag = false;
         return true;
     }
     return false;
 }
 
 uint64_t BLDCRPMSensor::getPulsePeriod() const {
     return pulsePeriod;
 }
 
 // ============================================================================
 // CONFIGURATION
 // ============================================================================
 void BLDCRPMSensor::setPulsesPerRev(float pulses) {
     pulsesPerRevolution = pulses;
 }
 
 void BLDCRPMSensor::setTimeout(uint32_t timeoutUs) {
     timeoutMicros = timeoutUs;
 }
 
 void BLDCRPMSensor::setUpdateInterval(uint32_t intervalMs) {
     updateIntervalMs = intervalMs;
 }
 
 void BLDCRPMSensor::setMinPulsePeriod(uint32_t minPeriodUs) {
     minPulsePeriodUs = minPeriodUs;
 }