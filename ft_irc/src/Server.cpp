/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: librahim <librahim@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/07/16 16:45:30 by librahim         ###   ########.fr       */
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
#include "../include/Server.hpp"

void    register_cl(std::vector<struct pollfd> *poll_fds, int cl_fd)
{
    struct pollfd cl;
    cl.fd = cl_fd;
    cl.events = POLLIN;
    poll_fds->push_back(cl);
    
}

void Server::setup()
{
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    struct addrinfo *res;
    int    ret = getaddrinfo(NULL, "6667", &hint, &res);
    if (ret < 0)
    {
        std::cerr << "ERROR1\n" << std::endl;
        exit(1);
    }
    this->server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (this->server_fd < 0)
    {
        std::cerr << "ERROR2\n" << std::endl;
        exit(1);
    }
    int opt = 1;
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0 )
    {
        std::cerr << "ERROR3\n" << std::endl;
        exit(1);
    }
    if (bind(this->server_fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        std::cerr << "ERROR4\n" << std::endl;
        exit(1);
    }
    if (listen(this->server_fd, 128) < 0)
    {
        std::cerr << "ERROR5\n" << std::endl;
        close(this->server_fd);
        exit(1);
    }
    freeaddrinfo(res);
    std::cout << "Server is listening on port 6667\n";
}

int main(int ac, char *av[])
{
    signal(SIGPIPE, SIG_IGN);
    Server s("6667", "password");
    s.setup();

    struct pollfd sv; // for the setup2 fct
    sv.fd = s.get_serv_fd();// for the setup2 fct
    sv.events = POLLIN;// for the setup2 fct
    s.poll_fds.push_back(sv);// for the setup2 fct



    int client_fd; // for the loop
    char buf[512]; // for the loop
    memset(buf, 0, 512); // for the loop    
    int size_cl =0; // for the loop
    struct sockaddr_in cl_adr; // for loop fct
    size_t bytes_readen; // for the loop 
    socklen_t cl_len = sizeof(cl_adr); // for the loop
    std::cout << "Beginning everything >>>"<< std::endl;
    while (true)
    {
        int ready = poll(s.poll_fds.data(), s.poll_fds.size(), 10);
        if (ready < 0)
        {
            std::cerr << "problem with poll"<<std::endl;
            continue;
        }
        if (s.poll_fds.at(0).revents & POLLIN)
        {
            std::cout << "server pollfd trigered"<<std::endl;
            client_fd = accept(s.get_serv_fd(), (struct sockaddr *)&cl_adr, &cl_len);
            if (client_fd < 0)
            {
                std::cerr << "ERROR\n" << std::endl;
                continue ;
            }
            size_cl++;
            std::cout << "New client added, " <<size_cl<<" in total now" <<std::endl;
            register_cl(&s.poll_fds, client_fd);
        }
        for (int i = 1; i <= size_cl; i++)
        {
                if (s.poll_fds.at(i).revents & POLLIN)
                {
                    memset(buf, 0, 512);
                    bytes_readen = recv(s.poll_fds.at(i).fd, &buf, sizeof(buf), 0);
                    if (bytes_readen > 0)
                    {
                        std::cout << "received message from client "<< i <<" : " << buf << std::endl;
                        std::string reply = "Welcome to ft_irc!\r\n";
                        send(s.poll_fds.at(i).fd, reply.c_str(), reply.length(), 0);
                    }
                }
        }
    }
    return 0;
}
