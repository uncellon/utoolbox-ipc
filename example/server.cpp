#include <iostream>
#include <ut/ipc/server.h>

int main(int argc, char* argv[]) {
    std::cout << "Enter 'q' to exit\n";

    UT::IPCServer server;
    server.onDataReceived.addEventHandler(
        UT::EventLoop::getMainInstance(), 
        [] (int id, std::shared_ptr<void> data, ssize_t bytes) {
            char* message = static_cast<char *>(data.get());
            message[bytes] = 0;
            std::cout << "Message received [" << id << "]: " << message << "\n";
        });
    server.onClientConnected.addEventHandler(
        UT::EventLoop::getMainInstance(),
        [] (int id) {
            std::cout << "New client connected [" << id << "]\n";
        });
    server.onClientDisconnected.addEventHandler(
        UT::EventLoop::getMainInstance(),
        [] (int id) {
            std::cout << "Client disconnected [" << id << "]\n";
        });
    server.start("example-server");

    std::string input;
    while (input != "q") {
        getline(std::cin, input);
    }

    return 0;
}