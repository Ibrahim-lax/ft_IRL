/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/13 16:13:24 by librahim          #+#    #+#             */
/*   Updated: 2025/08/22 17:44:36 by mjuicha          ###   ########.fr       */
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
#include <map>
#include "Client.hpp"
#include "Channel.hpp"

class Client;
class Channel;

class Server
{
    private:
        int server_fd;                         //  listening socket
        std::string port;               // Port 
        std::string pw;               // password
    public:
        int size_cl;
        std::vector<struct pollfd> poll_fds;   // vect of poll structs for cleints + server too
        Server(std::string port, std::string passw){this->port = port; this->pw = passw; this->size_cl = 0;};
        ~Server() {};
        void setup();                          // Bind, listen, etc.
        void run();                            // Main poll loop
        int get_serv_fd(){return server_fd;}
        std::string get_serv_pw(){return pw;}
        void closeClient() {};
        static std::vector<Channel*> channels;
        static std::vector<Client*> array_clients;
};

bool    password_check(int socket_fd, std::string message, std::string password);
void    nickname(Client *client, std::string message);
void    username(Client *client, std::string message);
void    delete_client(int i);
#endif
