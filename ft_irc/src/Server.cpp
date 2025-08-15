/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/08/15 10:45:13 by mjuicha          ###   ########.fr       */
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

void    register_cmnd(Client &client, std::string message, std::string password)
{
    std::string text;
    
    if (message.find("PASS") != std::string::npos)
        client.is_auth = password_check(client.socket_fd, message, password);
    else if (message.find("NICK") != std::string::npos)
    {
        if (!client.is_auth)
        {
            text = "You need to Authenticate first\r\n";
            send(client.socket_fd, text.c_str(), text.length(), 0);
            return ;
        }
        nickname(client, message);
    }
    else if (message.find("USER") != std::string::npos)
    {
        std::cout << "USER command found" << std::endl;
         if (!client.is_auth)
        {
            text = "You need to Authenticate first\r\n";
            send(client.socket_fd, text.c_str(), text.length(), 0);
            return;
        }
        username(client, message);
    }
    else
    {
        text = "Unknown command\r\n";
        send(client.socket_fd, text.c_str(), text.length(), 0);
    }
}

bool    password_check(int socket_fd, std::string message, std::string password)
{
    std::string pw;
    std::string text;

    pw = message.substr(5);
    pw.pop_back();
    if (password != pw)
        text = "wrong password\r\n";
    send(socket_fd, text.c_str(), text.length(), 0);
    return (password == pw);
}

bool    is_new_nickname(std::string nick, int socket_fd)
{
    std::string text;
    for (std::map<int, Client>::iterator it = Server::map_clients.begin(); it != Server::map_clients.end(); ++it)
    {
        if (it->second.nickname == nick)
        {
            text = "Nickname already in use\r\n";
            send(socket_fd, text.c_str(), text.length(), 0);
            return false;
        }
    }
    return true;
}
void    nickname(Client &client, std::string message)
{
    std::string nick = message.substr(5);
    nick.pop_back();
    client.nickname = nick;
    if (is_new_nickname(nick, client.socket_fd))
        saveinfo(client);
}

void    show_clients()
{
    std::cout << "Connected clients:" << std::endl;
    for (std::map<int, Client>::iterator it = Server::map_clients.begin(); it != Server::map_clients.end(); ++it)
    {
        std::cout << "Socket FD: " << it->first << ", Nickname: " << it->second.nickname << std::endl;
    }
}

void trim5(std::string message, std::vector<std::string> &array_string)
{
    size_t pos = 0;
    std::string token;
    while ((pos = message.find(" ")) != std::string::npos)
    {
        token = message.substr(0, pos);
        array_string.push_back(token);
        message.erase(0, pos + 1);
    }
}

void    username(Client &Cl, std::string message)
{
    std::vector<std::string> array_string;
    if (message.find("USER"))
        return ;
    trim5(message, array_string);
    for (size_t i = 0; i < array_string.size() + 1; i++)
    {
        std::cout << "array_string[" << i << "] = " << array_string[i] << std::endl;
    }
    
}

void    saveinfo(Client &Cl)
{
    Server::map_clients[Cl.socket_fd] = Cl;
}

void Server::run()
{
    int client_fd;
    char buf[512];
    memset(buf, 0, 512);
    int size_cl =0;
    struct sockaddr_in cl_adr;
    size_t bytes_readen;
    socklen_t cl_len = sizeof(cl_adr);
    // Client client;
    std::vector<Client> array_clients;
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
            register_cl(&this->poll_fds, client_fd); // vetor 
            array_clients.push_back(Client(client_fd));  // vector -> 0_ -> sizecl 
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
                    std::cout << buf;
                    if (!array_clients.at(i - 1).is_registered)
                        register_cmnd(array_clients.at(i - 1), buf, this->pw);
                    // // parsing here :
                    // // JOIN
                    // // PRIVMSG
                    std::string reply = "Welcome to ft_irc!\r\n";
                    send(this->poll_fds.at(i).fd, reply.c_str(), reply.length(), 0);
                    show_clients();
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
 