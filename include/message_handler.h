#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <mutex>
#include "logger.h"

class MessageHandler {
private:
    Logger logger;
    
public:
    MessageHandler();
    
    // Command parsing
    struct Message {
        enum Type {
            BROADCAST,
            PRIVATE,
            NICKNAME,
            HELP,
            LIST,
            QUIT,
            UNKNOWN
        };
        Type type;
        std::string sender;
        std::string target;     // For private messages
        std::string content;
    };
    
    // Parse incoming message
    Message parse_message(const std::string& raw_message, const std::string& sender_name);
    
    // Generate response
    std::string format_message(const Message& msg);
    std::string format_broadcast(const std::string& sender, const std::string& content);
    std::string format_private(const std::string& sender, const std::string& content);
    std::string format_system_message(const std::string& content);
    std::string format_user_list(const std::map<int, std::string>& users);
    
    // Command handlers
    std::string handle_help() const;
    std::string handle_list(const std::map<int, std::string>& users) const;
    
    // Validation
    bool is_valid_username(const std::string& username) const;
    bool is_valid_message(const std::string& message) const;
};

#endif // MESSAGE_HANDLER_H