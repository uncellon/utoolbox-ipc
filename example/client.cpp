#include <iostream>
#include <ut/ipc/client.h>

int main(int argc, char* argv[]) {
    UT::IPCClient client;
    client.onDataReceived.addEventHandler(
        UT::EventLoop::getMainInstance(), 
        [] (std::shared_ptr<void> data, ssize_t bytes) {
            char* message = static_cast<char *>(data.get());
            std::cout << message << std::endl;
        });
    client.onReadyChanged.addEventHandler(
        UT::EventLoop::getMainInstance(),
        [] (bool ready) {
            if (ready) {
                std::cout << "Connected to the server\n";
            } else {
                std::cout << "Server disconnected\n";
            }
        });
    client.start("example-server");

    std::cout << "Enter 'q' to exit\n";

    std::string input;
    while (true) {
        getline(std::cin, input);

        if (input == "q") {
            break;
        }

        client.send(input.c_str(), input.length());
    }

    return 0;
}