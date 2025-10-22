// =============================================================================
// stepcompress_improved.h - Klipper-Style Improved Step Compression
// Purpose: Enhanced step compression with velocity spike reduction
// =============================================================================

#pragma once

#include <vector>
#include <cstdint>

// =============================================================================
// Enhanced Step Chunk Structure
// =============================================================================
struct StepChunkImproved {
    uint32_t interval_us;     // Starting interval in microseconds
    int32_t add_us;           // Signed add applied after each step
    uint32_t count;           // Number of steps in this chunk
    uint32_t flags;           // Additional flags for optimization
    double max_velocity;      // Maximum velocity in this chunk
    double min_velocity;      // Minimum velocity in this chunk
};

// =============================================================================
// Improved StepCompressor Class (Klipper-style)
// =============================================================================
class StepCompressorImproved {
public:
    /**
     * @brief Enhanced trapezoidal compression with velocity spike reduction
     * @param total_steps Total number of steps in the move
     * @param start_vel Starting velocity (steps/sec)
     * @param cruise_vel Cruise velocity (steps/sec)
     * @param accel Acceleration (steps/sec²)
     * @param max_err_us Maximum allowed timing error per step (microseconds)
     * @param max_velocity_spike Maximum allowed velocity spike (steps/sec)
     * @return Vector of compressed step chunks
     */
    static std::vector<StepChunkImproved> compress_trapezoid_improved(
        uint32_t total_steps,
        double start_vel,
        double cruise_vel,
        double accel,
        double max_err_us = 20.0,
        double max_velocity_spike = 100.0);
    
    /**
     * @brief Klipper-style bisect compression algorithm
     * @param times Vector of absolute step times
     * @param start Start position in times array
     * @param end End position in times array
     * @param max_err_us Maximum allowed timing error
     * @return Compressed step chunk
     */
    static StepChunkImproved compress_bisect_add(
        const std::vector<uint64_t>& times,
        size_t start,
        size_t end,
        double max_err_us);
    
    /**
     * @brief Velocity spike detection and mitigation
     * @param chunks Vector of step chunks to analyze
     * @param max_spike Maximum allowed velocity spike
     * @return true if spikes are within tolerance
     */
    static bool detect_velocity_spikes(
        const std::vector<StepChunkImproved>& chunks,
        double max_spike);

private:
    /**
     * @brief Calculate velocity between two chunks
     * @param chunk1 First chunk
     * @param chunk2 Second chunk
     * @return Velocity difference
     */
    static double calculate_velocity_difference(
        const StepChunkImproved& chunk1,
        const StepChunkImproved& chunk2);
    
    /**
     * @brief Optimize chunk boundaries to reduce spikes
     * @param chunks Vector of chunks to optimize
     * @param max_spike Maximum allowed spike
     */
    static void optimize_chunk_boundaries(
        std::vector<StepChunkImproved>& chunks,
        double max_spike);
};
