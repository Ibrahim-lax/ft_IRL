/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/08/18 13:43:30 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

std::vector<Channel> Server::channels;
std::vector<Client> Server::array_clients;

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

bool client_joined_channel(Client &client, size_t i)
{
    for (size_t j = 0; j < Server::channels[i].clients.size(); j++)
    {
        if (Server::channels[i].clients[j]->socket_fd == client.socket_fd)
                return true;
    }
    return false;
}

bool banned_channel(Client &client, std::string &channel_name)
{
    std::cout << "inside banned_channel function" << std::endl;
    std::cout << "N: banned channels size: " << client.banned_channels.size() << std::endl;
    for (size_t i = 0; i < client.banned_channels.size(); i++)
    {
        std::cout << "Checking banned channel: " << client.banned_channels[i]->name << std::endl;
        if (client.banned_channels[i]->name == channel_name)
            return true;
    }
    return false;
}

bool    exist_channel(std::string name, Client &client)
{
    for (size_t i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i].name == name)
        {
            std::cout << "NAME OF CHANNEL: " << name << std::endl;
            std::cout << "CLIENT NICKNAME: " << client.nickname << std::endl;
            if (banned_channel(client, name))
            {
                std::string text = "You are banned from this channel\r\n";
                send(client.socket_fd, text.c_str(), text.length(), 0);
            }
            else if (!client_joined_channel(client, i))
            {
                client.channelsjoined.push_back(&Server::channels[i]);
                Server::channels[i].clients.push_back(&client);
            }
            return true;
        }
    }
    return false;
}


void    join(Client &client, std::string message)
{
    std::string text;
    std::string name;
    
    int b = message.find("#");
    if (b == std::string::npos)
    {
        text = "Invalid channel name\r\n";
        send(client.socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    name = message.substr(b + 1);
    b = name.find(" ");
    if (b != std::string::npos)
        name = name.substr(0, b);
    b = name.find("\n");
    if (b != std::string::npos)
        name = name.substr(0, b);
    if (exist_channel(name, client))
        return ;
    Channel channel;
    channel.name = name;
    channel.admin_socket_fd = client.socket_fd;
    client.channelsjoined.push_back(&channel);
    std::cout << "channel joined: " << channel.name << std::endl;
    std::cout << "channel joined: " << client.channelsjoined[0]->name << std::endl;
    channel.clients.push_back(&client);
    Server::channels.push_back(channel);
    text = "You have joined the channel: " + channel.name + "\r\n";
    send(client.socket_fd, text.c_str(), text.length(), 0);
}

//////////////////////////////////////////////////////////////////////
// void    msg(Client &client, std::string message)
// {
//     std::string text;
//     std::string name_channel = "channel2";
//     for (int i = 0; i < Server::channels.size(); i++)
//     {
//         if (Server::channels[i].name == name_channel)
//         {
//             for (size_t j = 0; j < Server::channels[i].clients.size(); j++)
//             {
//                 if (Server::channels[i].clients[j].socket_fd != client.socket_fd)
//                 {
//                     text = client.nickname + ": " + message + "\r\n";
//                     send(Server::channels[i].clients[j].socket_fd, text.c_str(), text.length(), 0);
//                 }
//             }
//             return ;
//         }
//     }
// }

std::string channel_name(std::string &message)
{
    std::string name;
    size_t b = message.find("#");
    if (b == std::string::npos)
        return "";
    message = message.substr(b + 1);
    size_t e = message.find(" ");
    if (e == std::string::npos)
        return "";
    name = message.substr(0, e);
    message = message.substr(e + 1);
    b = name.find("\n");
    if (b != std::string::npos)
        name = name.substr(0, b);
    return name;
}

std::string nickname_to_kick(std::string &message)
{
    std::string nick;
    size_t i = message.find(" ");
    if (i == std::string::npos)
    {
        i = message.find("\n");
        if (i == std::string::npos)
            return "";
        nick = message.substr(0, i);
        return nick;
    }
    nick = message.substr(0, i);
    return nick;
}

bool valid_channel(std::string &name, int *b)
{
    if (name.empty())
        return false;
    for (size_t i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i].name == name)
        {
            *b = i;
            return true;
        }
    }
    return false;
}

bool is_admin(Client &client, int i)
{
    if (Server::channels[i].admin_socket_fd == client.socket_fd)
        return true;
    return false;
}

bool valid_nickname(std::string &nick, int i, int *socket_fd)
{
    for (int j = 0; j < Server::channels[i].clients.size(); j++)
    {
        if (Server::channels[i].clients[j]->nickname == nick)
        {
            *socket_fd = Server::channels[i].clients[j]->socket_fd;
            return true;
        }
    }
    return false;    
}

void    kick(Client &client, std::string message)
{
    std::string text;
    int i = 0;
    int socket_fd = -1;
    std::string name_channel = channel_name(message);
    std::string nick_to_kick = nickname_to_kick(message);
    std::cout << "Channel name: " << name_channel << std::endl;
    std::cout << "Nickname: " << nick_to_kick << std::endl;
    if (!valid_channel(name_channel, &i))
    {
        text = "Invalid channel name\r\n";
        send(client.socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (!is_admin(client, i))
    {
        text = "You are not an admin of this channel\r\n";
        send(client.socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (!valid_nickname(nick_to_kick, i, &socket_fd))
    {
        text = "User not found in this channel\r\n";
        send(client.socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    Server::array_clients[socket_fd].banned_channels.push_back(&Server::channels[i]);
    text = "You have been kicked from the channel: " + Server::channels[i].name + "\r\n";
    send(Server::array_clients[socket_fd].socket_fd, text.c_str(), text.length(), 0);
    text = "You have kicked " + Server::array_clients[socket_fd].nickname + " from the channel: " + Server::channels[i].name + "\r\n";
    send(client.socket_fd, text.c_str(), text.length(), 0);
}

void    register_cmd(Client &client, std::string message, std::string password)
{
    std::string text;

    if (message.find("NICK") != std::string::npos)
        nickname(client, message);
    else if (message.find("JOIN") != std::string::npos)
    {
        join(client, message);
    }
    else if (message.find("KICK") != std::string::npos)
        kick(client, message);
    // else
    //     msg(client, message);
    
}

void    unregister_cmnd(Client &client, std::string message, std::string password)
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
    if (client.is_auth && client.is_nickname && client.is_username)
    {
        client.is_registered = true;
        text = "Welcome to the ft_irc Network\r\n";
        send(client.socket_fd, text.c_str(), text.length(), 0);
        std::cout << "CLIENT CONNECTED, ON FD: " << client.socket_fd << std::endl;
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

    for (int i = 0; i < Server::array_clients.size(); i++)
    {
        if (Server::array_clients[i].nickname == nick)
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
    client.is_nickname = true;
    if (is_new_nickname(nick, client.socket_fd))
        client.nickname = nick;
}

void    show_clients()
{
    for (int i = 0; i < Server::array_clients.size(); i++)
    {
        std::cout << "Client " << i + 1 << ": " << std::endl;
        std::cout << "Socket FD: " << Server::array_clients[i].socket_fd << std::endl;
        std::cout << "\t" << "Nickname: " << Server::array_clients[i].nickname << std::endl;
        std::cout << "\t" << "Username: " << Server::array_clients[i].username << std::endl;
        std::cout << "\t" << "Real Name: " << Server::array_clients[i].real_name << std::endl;
        std::cout << "\t" << "Is Authenticated: " << (Server::array_clients[i].is_auth ? "Yes" : "No") << std::endl;
        std::cout << "\t" << "Is Registered: " << (Server::array_clients[i].is_registered ? "Yes" : "No") << std::endl;
        std::cout << "----------------------------" << std::endl;
        std::cout << "\t" << "Channels Joined: ";
        if (Server::array_clients[i].channelsjoined.empty())
            std::cout << "None" << std::endl;
        else
        {
            for (size_t j = 0; j < Server::channels[i].clients.size(); j++)
            {
                std::cout << Server::channels[i].clients[j]->nickname << " ";
            }
            // for (size_t j = 0; j < Server::array_clients[i].channelsjoined.size(); j++)
            // {
            //     std::cout << Server::array_clients[i].channelsjoined[j]->name << " ";
            // }
            std::cout << std::endl;
        }
        std::cout << "-----------------------------" << std::endl;
        std::cout << "\t" << "Banned Channels: ";
        if (Server::array_clients[i].banned_channels.empty())
            std::cout << "None" << std::endl;
        else
        {
            for (size_t j = 0; j < Server::array_clients[i].banned_channels.size(); j++)
            {
                std::cout << Server::array_clients[i].banned_channels[j]->name << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------------------------" << std::endl;
    }
}

size_t skip_spaces(std::string message, size_t i)
{
    while (message[i] && message[i] == ' ')
        i++;
    return i;
}

void split_to_four(std::string message, std::vector<std::string> &array_string)
{
    size_t num = 0;
    size_t i = 5;
    size_t start = 0;
    std::string token;
    while(message[i] && message[i] == ' ')
        i++;
    start = i;
    while (message[i])
    {
        if ((message[i] == ' ' || message[i] == '\r' || message[i] == '\n') && num < 3)
        {
            token = message.substr(start, i - start);
            array_string.push_back(token);
            i = skip_spaces(message, i);
            start = i;
            num++;
        }
        if (!(message[i] == '\r' || message[i] == '\n') && num == 3)
        {
            while (message[i] && (message[i] != '\r' && message[i] != '\n'))
                i++;
            token = message.substr(start, i - start);
            if (token.empty())
                return ;
            array_string.push_back(token);
            return ;
        }
        else
            i++;
    }
}

bool is_valid_user(std::vector<std::string> &array_string)
{
    if (array_string.size() != 4)
        return false;
    return true;
}

std::string realname(std::string message)
{
    int b = 0;

    b = message.find(':');
    if (b != std::string::npos)
        return message.substr(b + 1);
    return message;
}

void    username(Client &client, std::string message)
{
    std::vector<std::string> array_string;
    if (message.find("USER"))
        return ;
    split_to_four(message, array_string);
    if (is_valid_user(array_string))
    {
        client.username = array_string[0];
        client.real_name = realname(array_string[3]);
        client.is_username = true;
    }
    else
    {
        std::string text = "Invalid USER command format\r\n";
        send(client.socket_fd, text.c_str(), text.length(), 0);
    }
    for (size_t i = 0; i < array_string.size(); i++)
    {
        std::cout << "array_string[" << i << "] = " << "'" <<array_string[i]<< "'" << std::endl;
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
    socklen_t cl_len = sizeof(cl_adr);
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
                        unregister_cmnd(array_clients.at(i - 1), buf, this->pw);
                    else
                        register_cmd(array_clients.at(i - 1), buf, this->pw);
                    // // parsing here :
                    // // JOIN
                    // // PRIVMSG
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
 