/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: librahim <librahim@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/07/13 19:35:43 by librahim         ###   ########.fr       */
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


void    register_cl(struct pollfd *a, int cl_fd, int count)
{
    a[count].fd = cl_fd;
    a[count].events = POLLIN;
}


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
    struct sockaddr_in cl_adr;
    socklen_t cl_len = sizeof(cl_adr);
    int size_cl =3;
    struct pollfd fds[size_cl];
    std::cout << "Registering client phase (currently max = "<< size_cl << ") >>>" <<  std::endl;
    int count = 0;
    while (count < size_cl)
    {
        client_fd = accept(sock_fd, (struct sockaddr *)&cl_adr, &cl_len);
        if (client_fd == -1)
        {
            std::cerr << "ERROR\n" << std::endl;
            continue ;
        }
        std::cout << " new client accepted." << std::endl;
        register_cl(fds, client_fd, count);
        count++;
    }
    std::cout << "Beginning the listening for messages phase >>>"<< std::endl;
    while (true)
    {
        int ready = poll(fds, size_cl, 10);
        for (int i = 0; i < size_cl; i++)
        {
                if (fds[i].revents & POLLIN)
                {
                    memset(buf, 0, 512);
                    bytes_readen = recv(fds[i].fd, &buf, sizeof(buf), 0);
                    if (bytes_readen > 0)
                    {
                        std::cout << "received message from client : "<< buf << std::endl;
                        std::string reply = "Welcome to ft_irc!\r\n";
                        send(fds[i].fd, reply.c_str(), reply.length(), 0);
                    }
                }
        }
    }
    freeaddrinfo(res);
    return 0;
}
