#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h> // getaddrinfo
#include <sys/socket.h>

int main()
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("127.0.0.1", "6667", &hints, &res) != 0)
    {
        perror("getaddrinfo");
        return 1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("connect");
        freeaddrinfo(res);
        close(sock);
        return 1;
    }
    std::string msg;
    while (1)
    {
        std::cout << "type msg to send :";
        std::getline(std::cin, msg);
        // std::cin >> msg;
        if (msg.empty())
            continue;
        send(sock, msg.c_str(), msg.size(), 0); 
        char buffer[1024];
        memset(buffer, 0, 1024);
        ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0)
            std::cout << "Server replied: " << buffer;
    }
    close(sock);
    freeaddrinfo(res);
    return 0;
}
