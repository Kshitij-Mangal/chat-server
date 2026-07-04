#include "../include/message_handler.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

MessageHandler::MessageHandler() : logger("handler.log", false) {}

// Parse incoming message
MessageHandler::Message MessageHandler::parse_message(const std::string& raw_message, 
                                                       const std::string& sender_name) {
    Message msg;
    msg.sender = sender_name;
    msg.type = Message::UNKNOWN;
    msg.content = raw_message;
    
    // Check if it's a command (starts with /)
    if (!raw_message.empty() && raw_message[0] == '/') {
        // Find the first space to get the command
        size_t first_space = raw_message.find(' ');
        std::string command = raw_message.substr(0, first_space);
        
        // Convert to lowercase for case-insensitive comparison
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        if (command == "/nick" || command == "/name") {
            msg.type = Message::NICKNAME;
            // Extract the nickname (everything after the command)
            if (first_space != std::string::npos) {
                std::string name = raw_message.substr(first_space + 1);
                // Trim leading/trailing spaces
                while (!name.empty() && name[0] == ' ') name.erase(0, 1);
                while (!name.empty() && name.back() == ' ') name.pop_back();
                msg.content = name;
            } else {
                msg.content = "";
            }
            logger.info("NICK PARSED: name='" + msg.content + "'");
        } 
        else if (command == "/msg" || command == "/whisper" || command == "/tell") {
            msg.type = Message::PRIVATE;
            
            logger.info("MSG COMMAND DETECTED: raw='" + raw_message + "'");
            
            if (first_space != std::string::npos) {
                // Get the part after the command
                std::string rest = raw_message.substr(first_space + 1);
                logger.info("MSG REST: '" + rest + "'");
                
                // Find the first space in the rest (to separate target and message)
                size_t second_space = rest.find(' ');
                
                if (second_space != std::string::npos) {
                    // Target is before the second space
                    msg.target = rest.substr(0, second_space);
                    // Message is after the second space
                    msg.content = rest.substr(second_space + 1);
                    // Trim leading spaces from content
                    while (!msg.content.empty() && msg.content[0] == ' ') {
                        msg.content.erase(0, 1);
                    }
                } else {
                    // No message content, only target specified
                    msg.target = rest;
                    msg.content = "";
                }
            } else {
                msg.target = "";
                msg.content = "";
            }
            
            logger.info("PRIVATE MSG PARSED: target='" + msg.target + "', content='" + msg.content + "'");
        } 
        else if (command == "/help") {
            msg.type = Message::HELP;
            msg.content = "";
        } 
        else if (command == "/list" || command == "/users") {
            msg.type = Message::LIST;
            msg.content = "";
        } 
        else if (command == "/quit" || command == "/exit" || command == "/bye") {
            msg.type = Message::QUIT;
            msg.content = "";
        } 
        else {
            msg.type = Message::UNKNOWN;
            msg.content = raw_message;
        }
    } else if (!raw_message.empty()) {
        msg.type = Message::BROADCAST;
        msg.content = raw_message;
    }
    
    return msg;
}

// Format broadcast message
std::string MessageHandler::format_broadcast(const std::string& sender, const std::string& content) {
    return "[" + sender + "]: " + content;
}

// Format private message
std::string MessageHandler::format_private(const std::string& sender, const std::string& content) {
    return "[PM from " + sender + "]: " + content;
}

// Format system message
std::string MessageHandler::format_system_message(const std::string& content) {
    return "[SYSTEM]: " + content;
}

// Handle help command
std::string MessageHandler::handle_help() const {
    std::string help = 
        "\n+---------------------------------------------+\n"
        "|          CHAT SERVER COMMANDS                |\n"
        "+-----------------------------------------------+\n"
        "| /help              - Show this help message   |\n"
        "| /list              - List all online users    |\n"
        "| /nick <name>       - Change your nickname     |\n"
        "| /msg <user> <msg>  - Send private message     |\n"
        "| /quit              - Disconnect from server   |\n"
        "+-----------------------------------------------+\n"
        "| Any other text - Broadcast to everyone         |\n"
        "+-----------------------------------------------+\n";
    return help;
}

// Handle list command
std::string MessageHandler::handle_list(const std::map<int, std::string>& users) const {
    std::string list = 
        "\n+-----------------------------------------------+\n"
        "|          ONLINE USERS                          |\n"
        "+-----------------------------------------------+\n";
    
    if (users.empty()) {
        list += "|   No users online                             |\n";
    } else {
        for (const auto& pair : users) {
            std::string name = pair.second;
            // Pad the name to align the border
            int padding = 37 - (int)name.length();
            if (padding < 0) padding = 0;
            list += "|   * " + name + std::string(padding, ' ') + "|\n";
        }
    }
    
    list += "+-----------------------------------------------+\n";
    list += "|   Total: " + std::to_string(users.size()) + " users online\n";
    list += "+-----------------------------------------------+\n";
    return list;
}

// Validate username
bool MessageHandler::is_valid_username(const std::string& username) const {
    if (username.empty() || username.length() > 20) {
        return false;
    }
    
    // Check for invalid characters
    for (char c : username) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }
    return true;
}

// Validate message
bool MessageHandler::is_valid_message(const std::string& message) const {
    return !message.empty() && message.length() <= 4096;
}