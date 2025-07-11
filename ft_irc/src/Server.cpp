/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: librahim <librahim@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/07/11 20:48:32 by librahim         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include <iostream>
#include <cstring>
#include <cstdlib>       // For exit()
#include <unistd.h>      // For close()
#include <netdb.h>       // For getaddrinfo(), addrinfo
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>       // For fcntl()
#include <poll.h>        // For poll()

int main()
{
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    struct addrinfo *res;
    int    ret = getaddrinfo(NULL, "6667", &hint, &res);
    if (ret == -1)
    {
        std::cerr << "ERROR\n" << std::endl;
        exit(1);
    }
    
    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd == -1)
    {
        std::cerr << "ERROR\n" << std::endl;
        exit(1);
    }
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 )
    {
        std::cerr << "ERROR\n" << std::endl;
        exit(1);
    }
    if (bind(sock_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        std::cerr << "ERROR\n" << std::endl;
        exit(1);
    }
    if (listen(sock_fd, 128) == -1)
    {
        std::cerr << "ERROR\n" << std::endl;
        close(sock_fd);
        exit(1);
    }
    std::cout << "Server is listening on port 6667\n";
    int client_fd;
    char buf[512];
    memset(buf, 0, 512);
    size_t bytes_readen;
    while (true)
    {
        struct sockaddr_in cl_adr;
        socklen_t cl_len = sizeof(cl_adr);
        client_fd = accept(sock_fd, (struct sockaddr *)&cl_adr, &cl_len);
        if (client_fd == -1)
        {
            std::cerr << "ERROR\n" << std::endl;
            continue ;
        }
        std::cout << " new client accepted." <<std::endl;
        while (true)
        {
            memset(buf, 0, 512);
            bytes_readen = recv(client_fd, &buf, sizeof(buf) -1, 0);
            if (bytes_readen > 0)
                std::cout << "received message from client : "<< buf << std::endl;
            std::string reply = ":irc.localhost 001 user :Welcome to ft_irc!\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        }
    }

    freeaddrinfo(res);
    return 1;
}