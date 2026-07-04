#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <mutex>
#include <ctime>
#include <thread>

class Logger {
private:
    std::ofstream log_file;
    std::mutex log_mutex;
    bool console_output;
    
public:
    Logger(const std::string& filename = "server.log", bool console = true) 
        : console_output(console) {
        log_file.open(filename, std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Warning: Could not open log file!" << std::endl;
        }
    }
    
    ~Logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }
    
    void log(const std::string& message, const std::string& level = "INFO") {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::string time_str = std::ctime(&time_t_now);
        time_str.pop_back(); // Remove newline
        
        // Format: [2024-01-15 10:30:45] [INFO] Message
        std::string formatted = "[" + time_str + "] [" + level + "] " + message + "\n";
        
        // Write to file
        if (log_file.is_open()) {
            log_file << formatted;
            log_file.flush(); // Force write to disk
        }
        
        // Write to console if enabled
        if (console_output) {
            std::cout << formatted;
        }
    }
    
    void info(const std::string& msg) { log(msg, "INFO"); }
    void warning(const std::string& msg) { log(msg, "WARNING"); }
    void error(const std::string& msg) { log(msg, "ERROR"); }
    void debug(const std::string& msg) { log(msg, "DEBUG"); }
};

#endif // LOGGER_H