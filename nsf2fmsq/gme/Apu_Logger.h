// APU Register Logger for NSF emulation
// Logs APU register writes with timing for lightweight playback

#ifndef APU_LOGGER_H
#define APU_LOGGER_H

#include "blargg_common.h"

#include <vector>
#include <stdio.h>

typedef int32_t nes_time_t;
typedef unsigned nes_addr_t;

// APU event types for INIT/PLAY tracking
enum apu_log_event_type {
    APU_EVENT_WRITE = 0,     // Normal APU register write
    APU_EVENT_INIT_START,    // INIT routine started
    APU_EVENT_INIT_END,      // INIT routine completed
    APU_EVENT_PLAY_START,    // PLAY routine started
    APU_EVENT_PLAY_END       // PLAY routine completed
};

// APU register write event
struct apu_log_entry_t {
    nes_time_t time;         // CPU cycle time when write occurred
    uint16_t addr;           // Register address (0x4000-0x4017) or event marker
    uint8_t data;            // Data written to register
    uint8_t event_type;      // Event type (apu_log_event_type)
    uint32_t frame_number;   // Frame number for PLAY events
    
    apu_log_entry_t() : time(0), addr(0), data(0), event_type(APU_EVENT_WRITE), frame_number(0) {}
    apu_log_entry_t(nes_time_t t, uint16_t a, uint8_t d, uint8_t et = APU_EVENT_WRITE, uint32_t fn = 0) 
        : time(t), addr(a), data(d), event_type(et), frame_number(fn) {}
};

// APU Logger class
class Apu_Logger {
public:
    Apu_Logger();
    ~Apu_Logger();
    
    // Enable/disable logging
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    // Log an APU register write
    void log_write(nes_time_t time, nes_addr_t addr, int data);
    
    // Log INIT/PLAY events
    void log_init_start(nes_time_t time);
    void log_init_end(nes_time_t time);
    void log_play_start(nes_time_t time, uint32_t frame);
    void log_play_end(nes_time_t time, uint32_t frame);
    
    // Clear all logged entries
    void clear();
    
    // Get number of logged entries
    size_t entry_count() const { return entries_.size(); }
    
    // Get logged entries (read-only access)
    const std::vector<apu_log_entry_t>& get_entries() const { return entries_; }
    
    // Save log to binary file
    bool save_binary(const char* filename) const;
    
    // Save log to text file (for debugging)
    bool save_text(const char* filename) const;
    
    // Load log from binary file
    bool load_binary(const char* filename);
    
    // Set memory limit for logged entries (0 = unlimited)
    void set_memory_limit(size_t max_entries) { max_entries_ = max_entries; }
    
    // Get memory usage in bytes
    size_t get_memory_usage() const;
    
    // Set time base for relative timing
    void set_time_base(nes_time_t base_time) { time_base_ = base_time; }
    void reset_time_base() { time_base_ = 0; }
    
    // Reset frame start time when CPU time is reset
    void reset_frame_start_time() { frame_start_time_ = 0; }
    
    // Get current frame number
    uint32_t get_current_frame() const { return current_frame_; }
    void increment_frame() { current_frame_++; }
    void reset_frame_counter() { current_frame_ = 0; }

private:
    bool enabled_;
    std::vector<apu_log_entry_t> entries_;
    size_t max_entries_;    // 0 = unlimited
    nes_time_t time_base_;  // For relative timing
    uint32_t current_frame_; // Current frame number
    nes_time_t frame_start_time_; // Start time of current frame
    
    // Helper function to check if address is valid APU register
    bool is_apu_addr(nes_addr_t addr) const;
};

// Global logger instance (optional)
extern Apu_Logger* g_apu_logger;

// Convenience functions
inline void apu_logger_enable() {
    if (g_apu_logger) g_apu_logger->set_enabled(true);
}

inline void apu_logger_disable() {
    if (g_apu_logger) g_apu_logger->set_enabled(false);
}

inline void apu_logger_clear() {
    if (g_apu_logger) g_apu_logger->clear();
}

inline void apu_log_write(nes_time_t time, nes_addr_t addr, int data) {
    if (g_apu_logger && g_apu_logger->is_enabled()) {
        g_apu_logger->log_write(time, addr, data);
    }
}

// Binary file format header
struct apu_log_header_t {
    char magic[8];          // "APULOG\0\0"
    uint32_t version;       // File format version (2 for INIT/PLAY support)
    uint32_t entry_count;   // Number of log entries
    uint32_t frame_count;   // Total number of frames
    uint32_t reserved[3];   // For future use
};

// C interface functions (declared here for both C and C++ compilation)
#ifdef __cplusplus
extern "C" {
#endif
    void apu_logger_init();
    void apu_logger_cleanup();
    void apu_logger_set_enabled(int enabled);
    int apu_logger_is_enabled();
    void apu_logger_log_write_c(int32_t time, unsigned addr, int data);
    int apu_logger_save_binary_c(const char* filename);
    int apu_logger_save_text_c(const char* filename);
    size_t apu_logger_get_entry_count();
#ifdef __cplusplus
}
#endif

void init_apu_logger();
Apu_Logger* get_apu_logger();

#endif // APU_LOGGER_H