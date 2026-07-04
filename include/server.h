#ifndef SERVER_H
#define SERVER_H

// Windows includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib") // Link with Winsock library
#else
    // Linux includes
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <signal.h>
#endif

#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include <cstring>
#include <atomic>
#include <algorithm>

#include "logger.h"
#include "message_handler.h"

// Windows compatibility macros
#ifdef _WIN32
    #define CLOSE_SOCKET(s) closesocket(s)
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
    #define CLOSE_SOCKET(s) close(s)
    #define SOCKET_ERROR_VALUE -1
    #define INVALID_SOCKET_VALUE -1
#endif

class ChatServer {
private:
    // Network stuff
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Client management
    std::vector<int> client_sockets;
    std::map<int, std::string> client_names;
    std::mutex clients_mutex;
    
    // Server state
    int port;
    std::atomic<bool> running;
    Logger logger;
    MessageHandler message_handler;
    
#ifdef _WIN32
    WSADATA wsaData; // Windows socket initialization data
#endif
    
public:
    ChatServer(int port, bool logging = true);
    ~ChatServer();
    
    bool initialize();
    void start();
    void stop();
    
    void handle_client(int client_fd);
    void broadcast_message(const std::string& message, int sender_fd);
    void private_message(const std::string& message, int sender_fd, const std::string& target_name);
    
    void remove_client(int client_fd);
    void set_client_name(int client_fd, const std::string& name);
    std::string get_client_name(int client_fd);
    
    bool is_running() const { return running.load(); }
    
    static void signal_handler(int signum);
    static ChatServer* instance;
    
private:
    void cleanup_winsock();
};

#endif // SERVER_H