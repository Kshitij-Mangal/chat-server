#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <thread>
#include <atomic>

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

class ChatClient {
private:
    int sock_fd;
    std::string server_ip;
    int port;
    std::atomic<bool> connected;
    std::thread receive_thread;
    
public:
    ChatClient(const std::string& ip, int port);
    ~ChatClient();
    
    bool connect_to_server();
    void disconnect();
    bool send_message(const std::string& message);
    void receive_messages();
    bool is_connected() const { return connected.load(); }
};

#endif // CLIENT_H