#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../include/server.h"
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
    #include <ws2tcpip.h>
#else
    #include <sys/select.h>
    #include <fcntl.h>
#endif

ChatServer* ChatServer::instance = nullptr;

// Constructor
ChatServer::ChatServer(int port, bool logging) 
    : port(port), running(false), logger("server.log", logging) {
    instance = this;
    
#ifdef _WIN32
    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        logger.error("WSAStartup failed: " + std::to_string(result));
    }
#endif
}

// Destructor
ChatServer::~ChatServer() {
    stop();
    if (server_fd > 0) {
        CLOSE_SOCKET(server_fd);
    }
    cleanup_winsock();
    instance = nullptr;
}

// Initialize server
bool ChatServer::initialize() {
    logger.info("Initializing server on port " + std::to_string(port));
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET_VALUE) {
        logger.error("Socket creation failed");
        return false;
    }
    
#ifdef _WIN32
    // Windows: Set socket options
    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) < 0) {
        logger.error("setsockopt failed");
        CLOSE_SOCKET(server_fd);
        return false;
    }
#else
    // Linux: Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        logger.error("setsockopt failed");
        return false;
    }
#endif
    
    // Setup address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        logger.error("Bind failed");
        CLOSE_SOCKET(server_fd);
        return false;
    }
    
    // Listen for connections
    if (listen(server_fd, 50) < 0) {
        logger.error("Listen failed");
        CLOSE_SOCKET(server_fd);
        return false;
    }
    
    logger.info("Server initialized successfully on port " + std::to_string(port));
    return true;
}

// Start server
void ChatServer::start() {
    if (!initialize()) {
        return;
    }
    
    running.store(true);
    logger.info("Server started. Waiting for connections...");
    
#ifdef _WIN32
    // Windows doesn't use signals the same way
#else
    // Set up signal handling for Linux
    signal(SIGINT, ChatServer::signal_handler);
    signal(SIGTERM, ChatServer::signal_handler);
#endif
    
    // Main loop: ONLY watches for new incoming connections here.
    // Each connected client is fully owned by its own dedicated
    // thread (handle_client), which does its own recv() loop and
    // its own disconnect handling. The main loop must NOT also
    // recv() from client sockets - doing so races with the
    // per-client thread's recv() on the same socket, causing
    // commands/messages to be silently dropped at random.
    while (running.load()) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        // Wait for activity with timeout
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, nullptr, nullptr, &tv);
        
        if (activity < 0) {
            if (running.load()) {
                logger.error("select() error");
            }
            break;
        }
        
        if (activity == 0) {
            continue; // timeout - just loop again to re-check running flag
        }
        
        // New connection waiting
        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);
            int new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
            
            if (new_socket == INVALID_SOCKET_VALUE) {
                if (running.load()) {
                    logger.error("Accept failed");
                }
                continue;
            }
            
            // Add client
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                client_sockets.push_back(new_socket);
                client_names[new_socket] = "User" + std::to_string(new_socket);
            }
            
            logger.info("New client connected. Socket: " + std::to_string(new_socket));
            
            // Send welcome message
            std::string welcome = "Welcome to the chat server!\n";
            welcome += "Type /help for commands\n";
            send(new_socket, welcome.c_str(), welcome.length(), 0);
            broadcast_message("Client " + std::to_string(new_socket) + " joined the chat", new_socket);
            
            // Create thread for client - this thread exclusively
            // owns all recv() calls on this socket from now on
            std::thread client_thread(&ChatServer::handle_client, this, new_socket);
            client_thread.detach();
        }
    }
    
    stop();
}

// Handle a single client
void ChatServer::handle_client(int client_fd) {
    char buffer[1024];
    std::string client_name = get_client_name(client_fd);
    
    logger.info("Starting handler for client " + std::to_string(client_fd));
    
    while (running.load()) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                logger.info("Client " + std::to_string(client_fd) + " disconnected gracefully");
            } else {
                logger.error("Error reading from client " + std::to_string(client_fd));
            }
            break;
        }
        
        std::string raw_message(buffer, bytes_read);
        raw_message.erase(std::remove(raw_message.begin(), raw_message.end(), '\n'), raw_message.end());
        raw_message.erase(std::remove(raw_message.begin(), raw_message.end(), '\r'), raw_message.end());
        
        if (raw_message.empty()) continue;
        
        logger.debug("Received from " + std::to_string(client_fd) + ": " + raw_message);
        
        // Parse message
        MessageHandler::Message msg = message_handler.parse_message(raw_message, client_name);
        
        switch (msg.type) {
            case MessageHandler::Message::BROADCAST:
                broadcast_message(msg.content, client_fd);
                break;
                
            case MessageHandler::Message::PRIVATE:
                private_message(msg.content, client_fd, msg.target);
                break;
                
            case MessageHandler::Message::NICKNAME:
                if (message_handler.is_valid_username(msg.content)) {
                    set_client_name(client_fd, msg.content);
                    client_name = msg.content;
                    std::string response = "Nickname set to: " + msg.content + "\n";
                    send(client_fd, response.c_str(), response.length(), 0);
                } else {
                    std::string error = "Invalid nickname. Use letters, numbers, and underscores only.\n";
                    send(client_fd, error.c_str(), error.length(), 0);
                }
                break;
                
            case MessageHandler::Message::HELP: {
                std::string help = message_handler.handle_help();
                send(client_fd, help.c_str(), help.length(), 0);
                break;
            }
                
            case MessageHandler::Message::LIST: {
                std::lock_guard<std::mutex> lock(clients_mutex);
                std::string list = message_handler.handle_list(client_names);
                send(client_fd, list.c_str(), list.length(), 0);
                break;
            }
                
            case MessageHandler::Message::QUIT:
                logger.info("Client " + std::to_string(client_fd) + " requested quit");
                remove_client(client_fd);
                broadcast_message("Client " + client_name + " left the chat", client_fd);
                CLOSE_SOCKET(client_fd);
                return;
                
            case MessageHandler::Message::UNKNOWN:
            default: {
                std::string error = "Unknown command. Type /help for commands.\n";
                send(client_fd, error.c_str(), error.length(), 0);
                break;
            }
        }
    }
    
    // Client disconnected
    remove_client(client_fd);
    broadcast_message("Client " + client_name + " disconnected", client_fd);
    CLOSE_SOCKET(client_fd);
}

// Broadcast message to all clients except sender
void ChatServer::broadcast_message(const std::string& message, int sender_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    std::string formatted = message + "\n";
    for (int client_fd : client_sockets) {
        if (client_fd != sender_fd) {
            send(client_fd, formatted.c_str(), formatted.length(), 0);
        }
    }
}

// Send private message
// Send private message
// Send private message
void ChatServer::private_message(const std::string& message, int sender_fd, const std::string& target_name) {
    logger.info("PRIVATE FUNCTION CALLED: message='" + message + "', target='" + target_name + "', sender_fd=" + std::to_string(sender_fd));
    
    if (message.empty()) {
        std::string error = "Cannot send empty message\n";
        send(sender_fd, error.c_str(), error.length(), 0);
        return;
    }
    
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    // Debug: List all clients
    logger.info("Current clients (" + std::to_string(client_names.size()) + "):");
    for (const auto& pair : client_names) {
        logger.info("  fd=" + std::to_string(pair.first) + " -> " + pair.second);
    }
    
    // Find target client
    int target_fd = -1;
    for (const auto& pair : client_names) {
        if (pair.second == target_name) {
            target_fd = pair.first;
            logger.info("Found target: " + target_name + " at fd " + std::to_string(target_fd));
            break;
        }
    }
    
    if (target_fd == -1) {
        std::string error = "User '" + target_name + "' not found\n";
        send(sender_fd, error.c_str(), error.length(), 0);
        logger.warning("Target not found: " + target_name);
        return;
    }
    
    // Get sender name
    std::string sender_name = get_client_name(sender_fd);
    
    // Send to target
    std::string formatted = "[PM from " + sender_name + "]: " + message + "\n";
    int sent = send(target_fd, formatted.c_str(), formatted.length(), 0);
    logger.info("Sent to target (" + std::to_string(target_fd) + "): " + std::to_string(sent) + " bytes");
    
    // Send confirmation to sender
    std::string confirm = "[PM to " + target_name + "]: " + message + "\n";
    send(sender_fd, confirm.c_str(), confirm.length(), 0);
    logger.info("Confirmation sent to sender");
}

// Remove client
void ChatServer::remove_client(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = std::find(client_sockets.begin(), client_sockets.end(), client_fd);
    if (it != client_sockets.end()) {
        client_sockets.erase(it);
    }
    client_names.erase(client_fd);
    logger.info("Client " + std::to_string(client_fd) + " removed");
}

// Set client name
void ChatServer::set_client_name(int client_fd, const std::string& name) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    client_names[client_fd] = name;
    logger.info("Client " + std::to_string(client_fd) + " set name to: " + name);
}

// Get client name
std::string ChatServer::get_client_name(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = client_names.find(client_fd);
    if (it != client_names.end()) {
        return it->second;
    }
    return "User" + std::to_string(client_fd);
}

// Stop server
void ChatServer::stop() {
    if (!running.load()) return;
    
    running.store(false);
    logger.info("Stopping server...");
    
    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (int client_fd : client_sockets) {
            CLOSE_SOCKET(client_fd);
        }
        client_sockets.clear();
        client_names.clear();
    }
    
    if (server_fd > 0) {
        CLOSE_SOCKET(server_fd);
    }
    
    logger.info("Server stopped");
}

// Clean up Winsock (Windows)
void ChatServer::cleanup_winsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Signal handler
void ChatServer::signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << std::endl;
    if (instance) {
        instance->stop();
    }
    exit(0);
}

// Entry point
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        std::cout << "Example: " << argv[0] << " 8080" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number" << std::endl;
        return 1;
    }

    ChatServer server(port, true);
    server.start();

    return 0;
}