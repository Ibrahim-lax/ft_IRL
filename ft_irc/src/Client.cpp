#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h> // getaddrinfo
#include <sys/socket.h>

int main() {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Connect to localhost:6667
    if (getaddrinfo("127.0.0.1", "6667", &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        return 1;
    }
    std::string msg;
    while (1)
    {
        std::getline(std::cin, msg);
        send(sock, msg.c_str(), msg.size(), 0); 
        // Read response
        char buffer[1024] = {0};
        ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0)
            std::cout << "Server replied: " << buffer;
    }
    close(sock);
    freeaddrinfo(res);
    return 0;
}
