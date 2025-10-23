/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/13 16:13:24 by librahim          #+#    #+#             */
/*   Updated: 2025/10/23 16:42:48 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP
#include <iostream>
#include <cstring>
#include <cstdlib>     
#include <unistd.h>     
#include <netdb.h>     
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>     
#include <poll.h>      
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
    public:
        int size_cl;
        static std::vector<struct pollfd> poll_fds;
        std::vector<std::string> buffs;
        Server(std::string port, std::string passw);
        static bool is_server_running;
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
        static void    clean_server();
};

bool    password_check(Client *client, std::vector<std::string> array_params, std::string password);
void    nickname(Client *client, std::string message, int *fd_bot);
void    username(Client *client, std::string message);
void    delete_client(int i);
#endif
