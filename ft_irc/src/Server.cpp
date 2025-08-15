/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/08/15 03:04:14 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
std::map<int, Client> Server::map_clients;

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
        std::cerr << "ERROR2\n" << gai_strerror(this->server_fd) << std::endl;
        exit(1);
    }
    int opt = 1;
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0 )
    {
        std::cerr << "ERROR3\n" << std::endl;
        exit(1);
    }
    if ((ret = bind(this->server_fd, res->ai_addr, res->ai_addrlen)) < 0)
    {
        std::cerr << "ERROR4\n" << gai_strerror(ret) << std::endl;
        exit(1);
    }
    if (listen(this->server_fd, 128) < 0)
    {
        std::cerr << "ERROR5\n" << std::endl;
        close(this->server_fd);
        exit(1);
    }
    struct pollfd sv; 
    sv.fd = this->get_serv_fd();
    sv.events = POLLIN;
    this->poll_fds.push_back(sv);
    std::cout << "Server is listening on port 6667\n";
    freeaddrinfo(res);
}

void    register_cmnd(Client &client)
{
    if (client.reg.message.find("PASS") != std::string::npos)
    {
        client.reg.is_auth = password_check(client.reg);
        std::cout << "IS AUTH: " << client.reg.is_auth << std::endl;
    }
    else if (client.reg.message.find("NICK") != std::string::npos)
    {
        std::cout << "IS AUTH: " << client.reg.is_auth << std::endl;
        if (!client.reg.is_auth)
        {
            std::string text = "You need to Authenticate first\r\n";
            send(client.reg.socket_fd, text.c_str(), text.length(), 0);
            return ;
        }
        nickname(client);
    }
    else if (client.reg.message.find("USER") != std::string::npos)
    {
        std::cout << "USER command found" << std::endl;
         if (!client.reg.is_auth)
        {
            std::string text = "You need to Authenticate first\r\n";
            send(client.reg.socket_fd, text.c_str(), text.length(), 0);
            return;
        }
        username(client.reg);
    }
    else
    {
        std::cout << "Unknown command: " << client.reg.message << std::endl;
    }
    std::cout << "2IS AUTH: " << client.reg.is_auth << std::endl;
}

bool    password_check(Registration &reg)
{
    int b = 0; 
    std::string pw;
    std::string text;
    b = reg.message.find("PASS");
    if (b != std::string::npos)
    {
        pw = reg.message.substr(5);
        pw.pop_back();
        if (reg.password == pw)
            text = "CORRECT PASSWORD\r\n";
        else
            text = "INCORRECT PASSWORD\r\n";
        send(reg.socket_fd, text.c_str(), text.length(), 0);
    }
    // const char *s = m.c_str();
    // while(s[i])
    // {
    //     if (s[i] == '\r')
    //         std::cout << "carriage return detected" << std::endl;
    //     if (s[i] == '\n')
    //         std::cout << "line feed detected" << std::endl;
    //     i++;
    // }
    return (reg.password == pw);
}
void    searchexistedsockets(Client &Cl,Registration &reg)
{
    std::map<int, Client>::iterator it = Server::map_clients.find(reg.socket_fd);
    if (it != Server::map_clients.end())
    {
        it->second.nickname = Cl.nickname;
    }
}

void    nickname(Client &client)
{
    int b = 0;
    b = client.reg.message.find("NICK");
    if (b != std::string::npos)
    {
        std::string nick = client.reg.message.substr(5);
        nick.pop_back();
        client.nickname = nick;
    }
    searchexistedsockets(client, client.reg);
    saveinfo(client, client.reg.socket_fd);
}

void    username(Registration &reg)
{}

void    saveinfo(Client &Cl, int socket_fd)
{
    Server::map_clients.insert(std::make_pair(socket_fd, Cl));
    for (std::map<int, Client>::iterator it = Server::map_clients.begin();
     it != Server::map_clients.end();
     ++it)
    {
        std::cout << "Client FD: " << it->first
                    << ", Nickname: " << it->second.nickname
                    << std::endl;
    }
}

void Server::run()
{
    int client_fd;
    char buf[512];
    memset(buf, 0, 512);
    int size_cl =0;
    struct sockaddr_in cl_adr;
    size_t bytes_readen;
    int old_client_fd = 0;
    socklen_t cl_len = sizeof(cl_adr);
    Client new_client;
    int i;
    while (true)
    {
        int ready = poll(this->poll_fds.data(), this->poll_fds.size(), 10);
        if (ready < 0)
        {
            std::cerr << "problem with poll"<<std::endl;
            continue;
        }
        if (this->poll_fds.at(0).revents & POLLIN)
        {
            std::cout << "server pollfd trigered"<<std::endl;
            client_fd = accept(this->get_serv_fd(), (struct sockaddr *)&cl_adr, &cl_len);
            if (client_fd < 0)
            {
                std::cerr << "ERROR\n" << std::endl;
                continue ;
            }
            size_cl++;
            std::cout << "User "<< size_cl << " joined the chat"<<std::endl; //size_cl<<" in total now" <<std::endl;
            register_cl(&this->poll_fds, client_fd);
            
        }
        i = 0;
        while (++i <= size_cl)
        {
            if (this->poll_fds.at(i).revents & POLLIN)
            {
                memset(buf, 0, 512);
                bytes_readen = recv(this->poll_fds.at(i).fd, &buf, sizeof(buf), 0);
                if (bytes_readen > 0)
                {
                    std::cout << "received message from client "<< i <<" :" << buf << std::endl;
                    if (this->poll_fds.at(i).fd != old_client_fd)
                    {
                        std::map<int, Client>::iterator it = Server::map_clients.find(this->poll_fds.at(i).fd);
                        if (it != Server::map_clients.end())
                            new_client = it->second;
                        else
                        {
                            Client Cl;
                            new_client = Cl;   
                        }
                        old_client_fd = this->poll_fds.at(i).fd;
                    }
                    new_client.reg.message = std::string(buf);
                    new_client.reg.password = this->get_serv_pw();
                    new_client.reg.socket_fd = this->poll_fds.at(i).fd;
                    std::cout << "0IS AUTH: " << new_client.reg.is_auth << std::endl;
                    register_cmnd(new_client);
                    std::cout << "3IS AUTH: " << new_client.reg.is_auth << std::endl;
                    
                    // parsing here :
                    // JOIN
                    // PRIVMSG
                    std::string reply = "Welcome to ft_irc!\r\n";
                    send(this->poll_fds.at(i).fd, reply.c_str(), reply.length(), 0);
                }
                else if (bytes_readen == 0)
                {
                    close(this->poll_fds.at(i).fd);
                    this->poll_fds.erase(this->poll_fds.begin() + i);
                    size_cl--;
                    std::cout << "User " << i << " has left the chat" << std::endl;
                    i--;
                    std::cout <<size_cl<<" users in total now" <<std::endl;
                }
            }
        }
    }
}                        
 