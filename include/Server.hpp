/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: librahim <librahim@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/13 16:13:24 by librahim          #+#    #+#             */
/*   Updated: 2025/10/10 22:22:02 by librahim         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP
#include <iostream>
#include <cstring>
#include <cstdlib>  
#include <unistd.h>    
#include <netdb.h>       // For getaddrinfo(), addrinfo
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>        // For poll()
#include <vector>
#include "Client.hpp"
#include "Channel.hpp"

#define COMMA 44
#define MAX_CHANNELS 10


class Client;
class Channel;

class Server
{
    private:
        int server_fd;   
        std::string port;        
        std::string pw;          
        long server_start_time;
    public:
        int size_cl;
        std::vector<struct pollfd> poll_fds;  
        std::vector<std::string> buffs;
        Server(std::string port, std::string passw);
        ~Server() {};
        void setup(); 
        void run(); 
        int get_serv_fd(){return server_fd;}
        std::string get_serv_pw() {return pw;}
        void closeClient() {};
        static std::vector<Channel*> channels;
        static std::vector<Client*> array_clients;
        
        void execute(Client *client, std::string &message, int i, int *fd_bot);
        std::string  handlebotCommand(std::string &cmd);
};

bool    password_check(Client *client, std::vector<std::string> array_params, std::string password);
void    nickname(Client *client, std::vector<std::string> array_params, int *fd_bot);
void    username(Client *client, std::string message);
void    delete_client(int i);
#endif
