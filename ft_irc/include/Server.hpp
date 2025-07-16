/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: librahim <librahim@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/13 16:13:24 by librahim          #+#    #+#             */
/*   Updated: 2025/07/16 16:55:06 by librahim         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP
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
#include <vector>

class Server
{
    private:
        int server_fd;                         // Main listening socket
        std::string port;               // Port the server is listening on
        std::string pw;               // password
        // std::vector<int> client_fds;           // Optional: list of client sockets (could be inferred from poll_fds)

        // struct sockaddr_in server_addr;        // Address for bind()
        // socklen_t addr_len;

        // bool running;                          // Server loop flag

    public:
        std::vector<struct pollfd> poll_fds;   // List of all FDs (clients + server)
        Server(std::string port, std::string passw){this->port = port; this->pw = passw;}
        ~Server() {};
        void setup();                          // Bind, listen, etc.
        void run();                            // Main poll loop
        void acceptNewClient() {};
        int get_serv_fd(){return server_fd;}
        void handleClientMessage(int client_fd) {};
        void closeClient(int client_fd) {};
};
#endif