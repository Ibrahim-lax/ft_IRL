/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/08/22 17:47:51 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

std::vector<Channel*> Server::channels;
std::vector<Client*> Server::array_clients;

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

bool client_joined_channel(Client *client, size_t i)
{
    for (size_t j = 0; j < Server::channels[i]->clients.size(); j++)
    {
        if (Server::channels[i]->clients[j]->socket_fd == client->socket_fd)
        {
            std::string text = "You are already joined to this channel\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return true;
        }
    }
    return false;
}

bool banned_channel(Client *client, std::string &channel_name)
{
    std::cout << "inside banned_channel function" << std::endl;
    std::cout << "N: banned channels size: " << client->banned_channels.size() << std::endl;
    for (size_t i = 0; i < client->banned_channels.size(); i++)
    {
        std::cout << "Checking banned channel: " << client->banned_channels[i]->name << std::endl;
        if (client->banned_channels[i]->name == channel_name)
            return true;
    }
    return false;
}

bool    join_invite_only(Client *client, size_t i)
{
    std::string text;
    if (client->invited_channels.empty())
        return false;
    for (int l = 0; l < client->invited_channels.size(); l++)
    {
        std::cout << "Checking invited channel: " << client->invited_channels[l]->name << std::endl;
        if (client->invited_channels[l]->name == Server::channels[i]->name)
        {
            std::cout << "INVITE ONLY SUCCESSFUL" << std::endl;
            return true;
        }
    }
    return false;
}

bool    exist_channel(std::string name, Client *client)
{
    for (size_t i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i]->name == name)
        {
            std::cout << "NAME OF CHANNEL: " << name << std::endl;
            std::cout << "CLIENT NICKNAME: " << client->nickname << std::endl;
            if (banned_channel(client, name))
            {
                std::string text = "You are banned from this channel\r\n";
                send(client->socket_fd, text.c_str(), text.length(), 0);
            }
            else if (!client_joined_channel(client, i))
            {
                if (Server::channels[i]->is_invite_only)
                {
                    std::cout << "iam inside invite only channel" << std::endl;
                    if (!join_invite_only(client, i))
                        return true;
                }
                Channel *channel = Server::channels[i];
                
                client->channelsjoined.push_back(channel);
                client->channelsjoined.back()->clients.push_back(client);
                // Server::channels[i]->clients.push_back(client);
                int j = Server::channels[i]->clients.size() - 1;
                // Server::channels[i].clients[j]->channelsjoined.push_back(&Server::channels[i]);
            }
            return true;
        }
    }
    return false;
}


void    join(Client *client, std::string message)
{
    std::string text;
    std::string name;
    
    int b = message.find("#");
    if (b == std::string::npos)
    {
        text = "Invalid channel name\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
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
    Channel *channel = new Channel();
    channel->name = name;
    channel->admin_socket_fd = client->socket_fd;
    // channel->clients.push_back(client);
    client->channelsjoined.push_back(channel);
    client->channelsjoined.back()->clients.push_back(client);
    Server::channels.push_back(channel);
    int i = Server::channels.size() - 1;
    // Server::channels[i]->clients.push_back(client);
    int j = Server::channels[i]->clients.size() - 1;
    // Server::channels[i].clients[j]->channelsjoined.push_back(&Server::channels[i]);
    text = "You have joined the channel: " + channel->name + "\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
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
        if (Server::channels[i]->name == name)
        {
            *b = i;
            return true;
        }
    }
    return false;
}

bool is_admin(Client *client, int i)
{
    if (Server::channels[i]->admin_socket_fd == client->socket_fd)
        return true;
    return false;
}

bool valid_nickname(std::string &nick, int i, int *j)
{
    for (int k = 0; k < Server::channels[i]->clients.size(); k++)
    {
        if (Server::channels[i]->clients[k]->nickname == nick)
        {
            *j = k;
            return true;
        }
    }
    return false;    
}

void    un_invite(Client *client, Channel *channel)
{
    for (size_t i = 0; i < client->invited_channels.size(); i++)
    {
        if (client->invited_channels[i]->name == channel->name)
        {
            client->invited_channels.erase(
                std::remove(client->invited_channels.begin(), client->invited_channels.end(), channel),
                client->invited_channels.end());
            return ;
        }
    }
}

void    kick(Client *client, std::string message)
{
    std::string text;
    int i = 0;
    int j = 0;
    std::string name_channel = channel_name(message);
    std::string nick_to_kick = nickname_to_kick(message);
    std::cout << "Channel name: " << name_channel << std::endl;
    std::cout << "Nickname: " << nick_to_kick << std::endl;
    if (!valid_channel(name_channel, &i))
    {
        text = "Invalid channel name\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (!is_admin(client, i))
    {
        text = "You are not an admin of this channel\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (!valid_nickname(nick_to_kick, i, &j))
    {
        text = "User not found in this channel\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (Server::channels[i]->clients[j]->socket_fd == client->socket_fd)
    {
        text = "You cannot kick yourself\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    Server::channels[i]->clients[j]->banned_channels.push_back(Server::channels[i]);
    Server::channels[i]->clients[j]->channelsjoined.erase(
        std::remove(Server::channels[i]->clients[j]->channelsjoined.begin(),
                    Server::channels[i]->clients[j]->channelsjoined.end(),
                    Server::channels[i]),
        Server::channels[i]->clients[j]->channelsjoined.end());
    Server::channels[i]->clients.erase(
        std::remove(Server::channels[i]->clients.begin(),
                    Server::channels[i]->clients.end(),
                    Server::channels[i]->clients[j]),
        Server::channels[i]->clients.end());
    un_invite(Server::channels[i]->clients[j], Server::channels[i]);
    text = "You have been kicked from the channel: " + Server::channels[i]->name + "\r\n";
    send(Server::channels[i]->clients[j]->socket_fd, text.c_str(), text.length(), 0);
    text = "You have kicked " + Server::channels[i]->clients[j]->nickname + " from the channel: " + Server::channels[i]->name + "\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
}

std::string nickname_to_invite(std::string &message)
{
    std::string nick;
    size_t i = message.find(" ");
    if (i != std::string::npos)
    {
        nick = message.substr(0, i);
        message = message.substr(i + 1);
        return nick;
    }
    else
        return "";
}

std::string channel_name_invite(std::string &message)
{
    std::string name;
    size_t i = message.find(" ");
    if (i == std::string::npos)
    {
        i = message.find("\n");
        if (i == std::string::npos)
            return "";
        name = message.substr(1, i - 1);
        return name;
    }
    name = message.substr(0, i);
    return name;
}

bool    user_not_found(std::string &nick, size_t *index)
{
    for (size_t i = 0; i < Server::array_clients.size(); i++)
    {
        if (Server::array_clients[i]->nickname == nick)
        {
            *index = i;
            return false;
        }
    }
    return true;
}

bool    channel_not_found(std::string &name_channel, Channel *&channel)
{
    for (size_t i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i]->name == name_channel)
        {
            channel = Server::channels[i];
            std::cout << "Channel found: " << channel->name << std::endl;
            return false;
        }
    }
    return true;
}

bool    already_seted(Channel *channel, Client client)
{
    for (size_t i = 0; i < client.invited_channels.size(); i++)
    {
        if (client.invited_channels[i]->name == channel->name)
            return true;
    }
    return false;
}

void    show_array_clients()
{
    for (int i = 0; i < Server::array_clients.size(); i++)
    {
        std::cout << "Client " << i + 1 << ": " << Server::array_clients[i]->nickname << std::endl;
        for (size_t j = 0; j < Server::array_clients[i]->invited_channels.size(); j++)
        {
            std::cout << "\t" << "Invited to channel: " << Server::array_clients[i]->invited_channels[j]->name << std::endl;
        }
    }
}

void    show_array_channels()
{
    for (int i = 0; i < Server::channels.size(); i++)
    {
        std::cout << "Channel " << i + 1 << ": " << Server::channels[i]->name << std::endl;
        for (int j = 0; j < Server::channels[i]->clients.size(); j++)
        {
            std::cout << "\t" << "Client: " << Server::channels[i]->clients[j]->nickname << std::endl;
            for (size_t k = 0; k < Server::channels[i]->clients[j]->invited_channels.size(); k++)
            {
                std::cout << "\t\t" << "Invited to channel: " << Server::channels[i]->clients[j]->invited_channels[k]->name << std::endl;
            }
        }
    }
}

bool    is_inviter_joined(Channel *channel, Client *client)
{
    for (size_t i = 0; i < channel->clients.size(); i++)
    {
        if (channel->clients[i]->socket_fd == client->socket_fd)
            return true;
    }
    return false;
}

bool    is_invited_joined(Channel *channel, Client *client)
{
    for (size_t i = 0; i < client->channelsjoined.size(); i++)
    {
        if (client->channelsjoined[i]->name == channel->name)
            return true;
    }
    return false;
}

void    un_banned_channel(Client *client, Channel *channel)
{
    for (int i = 0; i< client->banned_channels.size(); i++)
    {
        if (client->banned_channels[i]->name == channel->name)
        {
            client->banned_channels.erase(
                std::remove(client->banned_channels.begin(), client->banned_channels.end(), channel),
                client->banned_channels.end());
            return ;
        }   
    }
}

void    invite(Client *client, std::string &message)
{
    std::string text;
    message = message.substr(7);
    std::string nick_to_invite = nickname_to_invite(message);
    std::string name_channel = channel_name_invite(message);
    size_t i = 0;
    Channel *channel = NULL;
    
    std::cout << "####  Nickname to invite: " << nick_to_invite << std::endl;
    std::cout << "####  Channel name: " << name_channel << std::endl;
    if (user_not_found(nick_to_invite, &i))
    {
        text = "User not found\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (channel_not_found(name_channel, channel))
    {
        text = "you are invited to the channel: " + name_channel + "\r\n";
        send(Server::array_clients[i]->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (!is_inviter_joined(channel, client))
    {
        text = "You are not joined to this channel\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (is_invited_joined(channel, Server::array_clients[i]))
    {
        text = "this user is already joined to this channel\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    channel->is_invite_only = true; // Set the channel to invite only if not already set
    if (channel->is_invite_only && !(client->socket_fd == channel->admin_socket_fd))
    {
        text = "you are not allowed to invite users to this channel\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    std::cout << "{{{{{{{{{{{{{{{{{{{{{{{{{{{}}}}}}}}}}}}}}}}}}}}}}}}}}}" << std::endl;
    if (!already_seted(channel, *Server::array_clients[i]))
         Server::array_clients[i]->invited_channels.push_back(channel);
    un_banned_channel(Server::array_clients[i], channel);
    show_array_clients();
    show_array_channels();
    text = "You have invited " + Server::array_clients[i]->nickname + " to the channel: " + name_channel + "\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    text = "You have been invited to the channel: " + name_channel + "\r\n";
    send(Server::array_clients[i]->socket_fd, text.c_str(), text.length(), 0);
}

void    quit(Client *client, std::string message, Server *server, int i)
{
    std::string text;
    send(client->socket_fd, "Goodbye!\r\n", 10, 0);
    std::cout << "Client disconnected, FD: " << client->socket_fd << std::endl;
    close(server->poll_fds[i].fd);
    server->poll_fds.erase(server->poll_fds.begin() + i);
    delete_client(i);
    server->size_cl--;
}

void    register_cmd(Client *client, std::string message, std::string password, Server *server, int i)
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
    else if (message.find("INVITE") != std::string::npos)
        invite(client, message);
    else if(message.find("QUIT") != std::string::npos)
        quit(client, message, server, i);
    // else
    //     msg(client, message);
}

void    unregister_cmnd(Client *client, std::string message, std::string password)
{
    std::string text;
    
    if (message.find("PASS") != std::string::npos)
        client->is_auth = password_check(client->socket_fd, message, password);
    else if (message.find("NICK") != std::string::npos)
    {
        if (!client->is_auth)
        {
            text = "You need to Authenticate first\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return ;
        }
        nickname(client, message);
    }
    else if (message.find("USER") != std::string::npos)
    {
         if (!client->is_auth)
        {
            text = "You need to Authenticate first\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return;
        }
        username(client, message);
    }
    else
    {
        text = "Unknown command\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
    }
    if (client->is_auth && client->is_nickname && client->is_username)
    {
        client->is_registered = true;
        text = "Welcome to the ft_irc Network\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        std::cout << "CLIENT CONNECTED, ON FD: " << client->socket_fd << std::endl;
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
        if (Server::array_clients[i]->nickname == nick)
        {
            text = "Nickname already in use\r\n";
            send(socket_fd, text.c_str(), text.length(), 0);
            return false;
        }
    }
    return true;
}

void    nickname(Client *client, std::string message)
{
    std::string nick = message.substr(5);
    nick.pop_back();
    if (is_new_nickname(nick, client->socket_fd))
    {
        client->nickname = nick;
        client->is_nickname = true;
    }
}

void    show_channels()
{
    for (int i = 0; i < Server::channels.size(); i++)
    {
        std::cout << "Channel " << i + 1 << ": " << Server::channels[i]->name << std::endl;
        std::cout << "\t" << "Admin Socket FD: " << Server::channels[i]->admin_socket_fd << std::endl;
        std::cout << "\t" << "Clients in Channel: ";
        if (Server::channels[i]->clients.empty())
            std::cout << "None" << std::endl;
        else
        {
            for (size_t j = 0; j < Server::channels[i]->clients.size(); j++)
            {
                std::cout << Server::channels[i]->clients[j]->nickname << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------------------------" << std::endl;
    }
}

void    show_clients()
{
    for (int i = 0; i < Server::array_clients.size(); i++)
    {
        std::cout << "Client " << i + 1 << ": " << std::endl;
        std::cout << "Socket FD: " << Server::array_clients[i]->socket_fd << std::endl;
        std::cout << "\t" << "Nickname: " << Server::array_clients[i]->nickname << std::endl;
        std::cout << "\t" << "Username: " << Server::array_clients[i]->username << std::endl;
        std::cout << "\t" << "Real Name: " << Server::array_clients[i]->real_name << std::endl;
        std::cout << "\t" << "Is Authenticated: " << (Server::array_clients[i]->is_auth ? "Yes" : "No") << std::endl;
        std::cout << "\t" << "Is Registered: " << (Server::array_clients[i]->is_registered ? "Yes" : "No") << std::endl;
        std::cout << "----------------------------" << std::endl;
        std::cout << "\t" << "Channels Joined: ";
        if (Server::array_clients[i]->channelsjoined.empty())
            std::cout << "None" << std::endl;
        else
        {
            for (size_t j = 0; j < Server::array_clients[i]->channelsjoined.size(); j++)
            {
                std::cout << Server::array_clients[i]->channelsjoined[j]->name << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------------------------" << std::endl;
        std::cout << "\t" << "Banned Channels: ";
        if (Server::array_clients[i]->banned_channels.empty())
            std::cout << "None" << std::endl;
        else
        {
            for (size_t j = 0; j < Server::array_clients[i]->banned_channels.size(); j++)
            {
                std::cout << Server::array_clients[i]->banned_channels[j]->name << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------------------------" << std::endl;
    }
    show_channels();
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

void    username(Client *client, std::string message)
{
    std::vector<std::string> array_string;
    if (message.find("USER"))
        return ;
    split_to_four(message, array_string);
    if (is_valid_user(array_string))
    {
        client->username = array_string[0];
        client->real_name = realname(array_string[3]);
        client->is_username = true;
    }
    else
    {
        std::string text = "Invalid USER command format\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
    }
    for (size_t i = 0; i < array_string.size(); i++)
    {
        std::cout << "array_string[" << i << "] = " << "'" <<array_string[i]<< "'" << std::endl;
    }
}

bool channel_has_one_user(Channel *channel)
{
    if (channel->clients.size() == 1)
        return true;
    return false;
}

void    erase_channel_from_banned_channels(Channel *channel)
{
    for (size_t i = 0; i < Server::array_clients.size(); i++)
    {
        for (size_t j = 0; j < Server::array_clients[i]->banned_channels.size(); j++)
        {
            if (Server::array_clients[i]->banned_channels[j]->name == channel->name)
            {
                Server::array_clients[i]->banned_channels.erase(
                    std::remove(Server::array_clients[i]->banned_channels.begin(),
                                Server::array_clients[i]->banned_channels.end(), channel),
                    Server::array_clients[i]->banned_channels.end());
            }
        }
    }
}

void    erase_channel_from_invited_channels(Channel *channel)
{
    for (size_t i = 0; i < Server::array_clients.size(); i++)
    {
        for (size_t j = 0; j < Server::array_clients[i]->invited_channels.size(); j++)
        {
            if (Server::array_clients[i]->invited_channels[j]->name == channel->name)
            {
                Server::array_clients[i]->invited_channels.erase(
                    std::remove(Server::array_clients[i]->invited_channels.begin(),
                                Server::array_clients[i]->invited_channels.end(), channel),
                    Server::array_clients[i]->invited_channels.end());
            }
        }
    }
}

void    remove_channel(Channel *channel)
{
    for (int i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i]->name == channel->name)
        {
            Server::channels.erase(
                std::remove(Server::channels.begin(), Server::channels.end(), Server::channels[i]),
                Server::channels.end());
            erase_channel_from_banned_channels(channel);
            erase_channel_from_invited_channels(channel);
            channel->clients.clear();
            delete channel;
            return ;
        }
    }
}

bool    channel_has_multiple_clients_not_admin(Channel *channel, Client *client)
{
    if (channel->clients.size() > 1 && channel->admin_socket_fd != client->socket_fd)
        return true;
    return false;
}

bool    channel_has_multiple_clients_admin(Channel *channel, Client *client)
{
    if (channel->clients.size() > 1 && channel->admin_socket_fd == client->socket_fd)
        return true;
    return false;
}

void    reclame_channel(Channel *channel, Client *client)
{
    std::string text;
    for (size_t i = 0; i < channel->clients.size(); i++)
    {
        text = client->nickname + " has left the channel: " + channel->name + "\r\n";
        send(channel->clients[i]->socket_fd, text.c_str(), text.length(), 0);
    }
}

void    handle_channels(int i)
{
    for (int j = 0; j < Server::array_clients[i - 1]->channelsjoined.size(); j++)
    {
        std::cout << "Handling channel: " << Server::array_clients[i - 1]->channelsjoined[j]->name << std::endl;
        Channel *channel = Server::array_clients[i - 1]->channelsjoined[j];
        if (channel_has_one_user(channel))
        {
            remove_channel(channel);
            std::cout << "channel has only one user, removing it" << std::endl;
            Server::array_clients[i - 1]->channelsjoined.clear();
            Server::array_clients[i - 1]->invited_channels.clear();
            Server::array_clients[i - 1]->banned_channels.clear();
            return ;
        }
        else if (channel_has_multiple_clients_not_admin(channel, Server::array_clients[i -1]))
        {
            std::cout << "channel has multiple clients, not admin" << std::endl;
            reclame_channel(channel, Server::array_clients[i - 1]);
            channel->clients.erase(
                std::remove(channel->clients.begin(), channel->clients.end(), Server::array_clients[i - 1]),
                channel->clients.end());
        }
        else if (channel_has_multiple_clients_admin(channel, Server::array_clients[i - 1]))
        {
            std::cout << "channel has multiple clients, admin" << std::endl;
            reclame_channel(channel, Server::array_clients[i - 1]);
            channel->clients.erase(
                std::remove(channel->clients.begin(), channel->clients.end(), Server::array_clients[i - 1]),
                channel->clients.end());
            std::cout << "Old admin socket fd: " << channel->admin_socket_fd << std::endl;
            channel->admin_socket_fd = channel->clients[0]->socket_fd;
            std::string text = "New admin of the channel: " + channel->name + " is " + channel->clients[0]->nickname + "\r\n";
            send(channel->admin_socket_fd, text.c_str(), text.length(), 0);
            std::cout << "New admin socket fd: " << channel->admin_socket_fd << std::endl;
        }
    }
    Server::array_clients[i - 1]->channelsjoined.clear();
    Server::array_clients[i - 1]->invited_channels.clear();
    Server::array_clients[i - 1]->banned_channels.clear();   
}

void    delete_client(int i)
{
    std::cout << "Deleting client " << i << std::endl;
    Client *client = Server::array_clients[i - 1];
    handle_channels(i);
    Server::array_clients.erase(
        std::remove(Server::array_clients.begin(), Server::array_clients.end(), Server::array_clients[i - 1]),
        Server::array_clients.end());
    delete client;
}

void Server::run()
{
    int client_fd;
    char buf[512];
    memset(buf, 0, 512);
    size_cl = 0;
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
            array_clients.push_back(new Client(client_fd));  // vector -> 0_ -> sizecl 
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
                    if (!array_clients.at(i - 1)->is_registered)
                        unregister_cmnd(array_clients.at(i - 1), buf, this->pw);
                    else
                        register_cmd(array_clients.at(i - 1), buf, this->pw, this, i);
                    // // parsing here :
                    // // JOIN
                    // // PRIVMSG
                    show_clients();
                }
                else if (bytes_readen == 0)
                {
                    close(this->poll_fds.at(i).fd);
                    this->poll_fds.erase(this->poll_fds.begin() + i);
                    delete_client(i);
                    size_cl--;
                    std::cout << "User " << i << " has left the chat" << std::endl;
                    i--;
                    std::cout <<size_cl<<" users in total now" <<std::endl;
                }
            }
        }
    }
}                        
