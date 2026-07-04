#include <iostream>
#include <string>
#include <thread>
#include <chrono>

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

int main() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║      CLIENT CONNECTION TESTS        ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "❌ Socket creation failed" << std::endl;
        return 1;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "❌ Invalid address" << std::endl;
        return 1;
    }
    
    std::cout << "🔗 Attempting to connect to server..." << std::endl;
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "❌ Connection failed" << std::endl;
        std::cout << "💡 Make sure server is running: ./server.exe 8080" << std::endl;
    } else {
        std::cout << "✅ Successfully connected to server!" << std::endl;
        
        // Send test message
        std::string test_msg = "Hello from test client!";
        send(sock, test_msg.c_str(), test_msg.length(), 0);
        std::cout << "📤 Sent: " << test_msg << std::endl;
        
        // Receive response
        char buffer[1024] = {0};
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            std::cout << "📥 Received: " << buffer << std::endl;
        }
        
        // Clean up
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        std::cout << "✅ Test completed!" << std::endl;
    }
    
    return 0;
}