#include <iostream>
#include "../include/server.h"
#include "../include/logger.h"

void test_server_initialization() {
    std::cout << "🧪 Testing server initialization..." << std::endl;
    
    ChatServer server(8080, true);
    bool init = server.initialize();
    
    if (init) {
        std::cout << "✅ Server initialized successfully" << std::endl;
    } else {
        std::cout << "❌ Server initialization failed" << std::endl;
    }
}

void test_server_start_stop() {
    std::cout << "🧪 Testing server start/stop..." << std::endl;
    
    ChatServer server(8081, true);
    if (server.initialize()) {
        std::cout << "Starting server in background..." << std::endl;
        std::thread server_thread([&server]() {
            server.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        std::cout << "Stopping server..." << std::endl;
        server.stop();
        
        if (server_thread.joinable()) {
            server_thread.join();
        }
        std::cout << "✅ Server start/stop test passed" << std::endl;
    } else {
        std::cout << "❌ Server initialization failed" << std::endl;
    }
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║      CHAT SERVER TESTS              ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";
    
    test_server_initialization();
    test_server_start_stop();
    
    std::cout << "\n✅ All tests completed!" << std::endl;
    return 0;
}