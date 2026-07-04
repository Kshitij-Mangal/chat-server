# Makefile for Windows with MinGW
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -O2 -D_WIN32_WINNT=0x0600 -D_WIN32
LDFLAGS = -lws2_32 -pthread -static

# Source files
SERVER_SRC = src/server.cpp src/thread_pool.cpp src/message_handler.cpp
CLIENT_SRC = src/client.cpp

# Object files
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Targets
TARGET_SERVER = server.exe
TARGET_CLIENT = client.exe

# Colors for output
GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[0;33m
NC = \033[0m # No Color

# Default target
all: $(TARGET_SERVER) $(TARGET_CLIENT)
	@echo "$(GREEN)✅ Build complete!$(NC)"
	@echo "$(YELLOW)To run server: ./server.exe 8080$(NC)"
	@echo "$(YELLOW)To run client: ./client.exe localhost 8080$(NC)"

# Build server
$(TARGET_SERVER): $(SERVER_OBJ)
	@echo "$(YELLOW)🔨 Linking server...$(NC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✅ Server built: $(TARGET_SERVER)$(NC)"

# Build client
$(TARGET_CLIENT): $(CLIENT_OBJ)
	@echo "$(YELLOW)🔨 Linking client...$(NC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✅ Client built: $(TARGET_CLIENT)$(NC)"

# Compile rules
%.o: %.cpp
	@echo "$(YELLOW)📝 Compiling $<...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	@echo "$(YELLOW)🧹 Cleaning...$(NC)"
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(TARGET_SERVER) $(TARGET_CLIENT)
	rm -f *.log
	@echo "$(GREEN)✅ Clean complete$(NC)"

# Run server
run-server: $(TARGET_SERVER)
	@echo "$(GREEN)🚀 Starting server on port 8080...$(NC)"
	./$(TARGET_SERVER) 8080

# Run client
run-client: $(TARGET_CLIENT)
	@echo "$(GREEN)🚀 Starting client...$(NC)"
	./$(TARGET_CLIENT) localhost 8080

# Test with multiple clients
test: $(TARGET_SERVER) $(TARGET_CLIENT)
	@echo "$(YELLOW)🧪 Starting test...$(NC)"
	@echo "Opening 3 terminals:"
	@echo "Terminal 1: ./server.exe 8080"
	@echo "Terminal 2: ./client.exe localhost 8080"
	@echo "Terminal 3: ./client.exe localhost 8080"

.PHONY: all clean run-server run-client test