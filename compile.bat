@echo off
set MINGW=C:\Users\KSHITIJ\winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64ucrt-11.0.1-r8\mingw64

"%MINGW%\bin\g++.exe" -std=c++17 -mconsole -static -static-libgcc -static-libstdc++ -Iinclude -o server.exe src/server.cpp src/message_handler.cpp src/thread_pool.cpp src/logger.cpp -lws2_32 -pthread -D_WIN32_WINNT=0x0600

"%MINGW%\bin\g++.exe" -std=c++17 -mconsole -static -static-libgcc -static-libstdc++ -Iinclude -o client.exe src/client.cpp -lws2_32 -pthread -D_WIN32_WINNT=0x0600

pause