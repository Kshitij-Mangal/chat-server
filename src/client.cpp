#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <atomic>
#include <signal.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

std::atomic<bool> running(true);
int client_fd = 0;

// Signal handler for clean exit
void signal_handler(int signum) {
    std::cout << "\nDisconnecting..." << std::endl;
    running.store(false);
    if (client_fd > 0) {
#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }
    exit(0);
}

// Receive messages from server
void receive_messages(int sock) {
    char buffer[4096];
    
    while (running.load()) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            if (running.load()) {
                std::cout << "\nDisconnected from server" << std::endl;
                running.store(false);
            }
            break;
        }
        
        buffer[bytes_read] = '\0';
        std::cout << buffer;
        std::cout.flush();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        std::cout << "Example: " << argv[0] << " localhost 8080" << std::endl;
        return 1;
    }
    
    const char* server_ip = argv[1];
    int port = std::stoi(argv[2]);
    
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif
    
    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }
    
    // Setup server address
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    // Convert IP address - Windows compatible
#ifdef _WIN32
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid address" << std::endl;
        return 1;
    }
#else
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return 1;
    }
#endif
    
    // Connect to server
    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to chat server!" << std::endl;
    std::cout << "Type /help for commands" << std::endl;
    std::cout << "==========================" << std::endl;
    
    // Start receive thread
    std::thread receive_thread(receive_messages, client_fd);
    
    // Main loop for sending messages
    std::string input;
    while (running.load()) {
        std::getline(std::cin, input);
        
        if (!running.load()) break;
        
        // Check for /quit command
        if (input == "/quit" || input == "/exit") {
            running.store(false);
            break;
        }
        
        if (!input.empty()) {
            int sent = send(client_fd, input.c_str(), input.length(), 0);
            if (sent < 0) {
                std::cerr << "Failed to send message" << std::endl;
                break;
            }
        }
    }
    
    // Clean up
    running.store(false);
    if (client_fd > 0) {
#ifdef _WIN32
        closesocket(client_fd);
        WSACleanup();
#else
        close(client_fd);
#endif
    }
    if (receive_thread.joinable()) {
        receive_thread.join();
    }
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}

// WinMain wrapper for Windows
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    return main(__argc, __argv);
}
#endif