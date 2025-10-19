/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: librahim <librahim@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 20:48:30 by librahim          #+#    #+#             */
/*   Updated: 2025/10/19 22:43:51 by librahim         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

std::vector<Channel*> Server::channels;
std::vector<Client*> Server::array_clients;


Server::Server(std::string port, std::string passw)
{
    this->port = port;
    this->pw = passw;
    this->size_cl = 0;
}
size_t skip_spaces(std::string message, size_t i);

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
    int    ret = getaddrinfo("127.0.0.1", this->port.c_str(), &hint, &res);
    if (ret < 0)
    {
        std::cerr << "ERROR1\n" << std::endl;
        freeaddrinfo(res);
        exit(1);
    }
    this->server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (this->server_fd < 0)
    {
        std::cerr << "ERROR2\n" << gai_strerror(this->server_fd) << std::endl;
        freeaddrinfo(res);
        exit(1);
    }
    if (fcntl(this->server_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "ERROR2\n" << gai_strerror(this->server_fd) << std::endl;
        close(this->server_fd);
        freeaddrinfo(res);
        exit(1);
    }
    int opt = 1;
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0 )
    {
        std::cerr << "ERROR3\n" << std::endl;
        freeaddrinfo(res);
        close(this->server_fd);
        exit(1);
    }
    if ((ret = bind(this->server_fd, res->ai_addr, res->ai_addrlen)) < 0)
    {
        std::cerr << "ERROR4\n" << gai_strerror(ret) << std::endl;
        close(this->server_fd);
        freeaddrinfo(res);
        exit(1);
    }
    if (listen(this->server_fd, 128) < 0)
    {
        std::cerr << "ERROR5\n" << std::endl;
        close(this->server_fd);
        freeaddrinfo(res);
        exit(1);
    }
    struct pollfd sv; 
    sv.fd = this->get_serv_fd();
    sv.events = POLLIN;
    this->poll_fds.push_back(sv);
    std::cout << "Server is listening on port " << this->port << std::endl;
    freeaddrinfo(res);
}

bool client_joined_channel(Client *client, size_t i)
{
    for (size_t j = 0; j < Server::channels[i]->clients.size(); j++)
    {
        if (Server::channels[i]->clients[j]->socket_fd == client->socket_fd)
        {
            std::string text = ":localhost 443 " + client->nickname + " #" + Server::channels[i]->name + " :is already on channel\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return true;
        }
    }
    return false;
}

bool    key_check(Client *client, std::string &key, int socket_fd, Channel *channel)
{
    std::string text;
    if (channel->is_password_required)
    {
        if (key != channel->password)
        {
            text = ":localhost 475 " + client->nickname + " #" + channel->name + " :Cannot join channel (+k)\r\n";
            send(socket_fd, text.c_str(), text.length(), 0);
            return false;
        }
    }
    return true;
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

bool    is_client_invited(Channel *channel, Client *client)
{
    for (size_t i = 0; i < client->invited_channels.size(); i++)
    {
        if (client->invited_channels[i]->name == channel->name)
            return true;
    }
    return false;
}

bool invite_only(Channel *channel, Client *client)
{
    if (channel->is_invite_only)
    {
        if (!is_client_invited(channel, client))
        {
            std::string text;
            text = ":localhost 473 " + client->nickname + " #" + channel->name + " :Cannot join channel (+i)\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return true;  // **block the join**
        }
    }
    return false;  // join allowed
}

bool    channel_full(Channel *channel, Client *client)
{
    std::string text;
    if (channel->is_limit_set)
    {
        if (channel->clients.size() >= (unsigned int) channel->limit)
        {
            text = ":localhost 471 " + client->nickname + " #" + channel->name + " :Cannot join channel (+l)\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return true;
        }
    }
    return false;
}

void    send_join_messages(Channel *channel, Client *client)
{
    std::string text;
    text = ":" + client->nickname + "!" + client->username + "@localhost JOIN #" + channel->name + "\r\n";
    for (size_t i = 0; i < channel->clients.size(); i++)
        send(channel->clients[i]->socket_fd, text.c_str(), text.length(), 0);
}

std::string     get_symbol(Channel *channel)
{
    if (channel->is_invite_only)
        return "*";
    if (channel->is_password_required)
        return "@";
    return "=";
}

void    namreply(Client *client, Channel *channel)
{
    std::string symbol = get_symbol(channel);
    std::vector<std::string> array_operators;
    std::vector<std::string> array_members;
    std::string tmp;
    std::string text = ":localhost 353 " + client->nickname + " " + symbol + " #" + channel->name + " :";
    for (size_t i = 0; i < channel->clients.size(); i++)
    {
        tmp = "@";
        if (channel->isOperator(channel->clients[i]->socket_fd))
        {
            tmp += channel->clients[i]->nickname;
            array_operators.push_back(tmp);
            continue;
        }
        array_members.push_back(channel->clients[i]->nickname);
    }
    for (size_t i = 0; i < array_operators.size(); i++)
    {
        text += array_operators[i];
        if (i != array_operators.size() - 1 || !array_members.empty())
            text += " ";
    }
    for (size_t i = 0; i < array_members.size(); i++)
    {
        text += array_members[i];
        if (i != array_members.size() - 1)
            text += " ";
    }
    text += "\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    text = ":localhost 366 " + client->nickname + " #" + channel->name + " :End of /NAMES list\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
}

bool    exist_channel(std::string &name, Client *client, std::string &key)
{
    for (size_t i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i]->name == name)
        {
            if (client_joined_channel(client, i))
                return true;
            if (channel_full(Server::channels[i], client))
                return true;
            if (!key_check(client, key, client->socket_fd, Server::channels[i]))
                return true;
            if (invite_only(Server::channels[i], client))
                return true;
            un_invite(client, Server::channels[i]);
            Channel *channel = Server::channels[i];
            std::string text;
            client->channelsjoined.push_back(channel);
            client->channelsjoined.back()->clients.push_back(client);
            send_join_messages(channel, client);
            if (channel->topic_join_msg)
            {
                text = ":localhost 332 " + client->nickname + " #" + channel->name + " :" + channel->topic + "\r\n";
                send(client->socket_fd, text.c_str(), text.length(), 0);
            }
            namreply(client, channel);
            return true;
        }
    }
    return false;
}

void parse_join_parameters(std::string &message, std::vector<std::string> &array_channels, std::vector<std::string> &array_keys)
{
    int i = 0;
    std::string channel = "";
    
    i = skip_spaces(message, i);
    if (!message[i])
    {
        message = "";
        return ;
    }
    while (message[i] && message[i] != ' ')
    {
        if (message[i] == COMMA)
        {
            array_channels.push_back(channel);
            channel = "";
        }
        else
            channel.push_back(message[i]);
        i++;
    }
    if (message[i - 1] != COMMA && !channel.empty())
        array_channels.push_back(channel);
    i = skip_spaces(message, i);
    if (!message[i])
        return ;
    std::string key = "";
    while (message[i] && message[i] != ' ')
    {
        if (message[i] == COMMA)
        {
            array_keys.push_back(key);
            key = "";
        }
        else
            key.push_back(message[i]);
        i++;
    }
    if (message[i - 1] != COMMA && !key.empty())
        array_keys.push_back(key);
}

bool    is_illegal(std::string message)
{
    std::string illegal_chars = " @#!*?:\t\r\n";
    return message.find_first_of(illegal_chars) != std::string::npos;
}

bool    is_valid_mask(std::string &channel, Client *client)
{
    std::string text;
    if (channel[0] == '#' && !is_illegal(channel.substr(1)) && channel.length() != 1)
        return true;
    text = ":localhost 476 " + client->nickname + " " + channel + " :Bad Channel Mask\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    return false;
}

void    join_channel(Client *client, std::string &channel_name, std::string &key)
{
    std::string text;
    channel_name = channel_name.substr(1);
    if (exist_channel(channel_name, client, key))
        return ;
    Channel *channel = new Channel();
    channel->name = channel_name;
    channel->operators.push_back(client->socket_fd);
    client->channelsjoined.push_back(channel);
    client->channelsjoined.back()->clients.push_back(client);
    Server::channels.push_back(channel);
    text = ":" + client->nickname + "!" + client->username + "@localhost JOIN #" + channel->name + "\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    namreply(client, channel);
}

bool    max_channel_reached(Client *client, std::string &channelname)
{
    std::string text;
    if (client->channelsjoined.size() >= MAX_CHANNELS)
    {
        text = ":localhost 405 " + client->nickname + " " + channelname + " :You have joined too many channels\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return true;
    }
    return false;
}

std::string get_key(std::vector<std::string> &array_keys, unsigned int i)
{
    if (i < array_keys.size())
        return array_keys[i];
    return "";
}

void    join(Client *client, std::string &message, std::vector<std::string> &array_params)
{
    std::string text;
    std::vector<std::string> array_channels;
    std::vector<std::string> array_keys;
    std::string key;
    
    if (array_params.size() > 2)
    {
        text = ":localhost 461 " + client->nickname + " JOIN :Too many parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    parse_join_parameters(message, array_channels, array_keys);
    if (message == "")
    {
        text = ":localhost 461 " + client->nickname + " JOIN :Not enough parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    for (size_t i = 0; i < array_channels.size(); i++)
    {
        if (max_channel_reached(client, array_channels[i]))
            continue;
        key = get_key(array_keys, i);
        if (is_valid_mask(array_channels[i], client))
            join_channel(client, array_channels[i], key);
    }
}

bool valid_channel(std::string &channel_name, Client *client, int *i)
{
    if (!is_valid_mask(channel_name, client))
        return false;
    channel_name = channel_name.substr(1);
    for (unsigned int j = 0; j < Server::channels.size(); j++)
    {
        if (Server::channels[j]->name == channel_name)
        {
            *i = j;
            return true;
        }
    }
    std::string text;
    text = ":localhost 403 " + client->nickname + " #" + channel_name + " :No such channel\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    return false;
}

bool    is_client_joined(Client *client, int i)
{
    for (size_t j = 0; j < Server::channels[i]->clients.size(); j++)
    {
        if (Server::channels[i]->clients[j]->socket_fd == client->socket_fd)
            return true;
    }
    std::string text;
    text = ":localhost 442 " + client->nickname + " #" + Server::channels[i]->name + " :You're not on that channel\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    return false;
}

bool    is_client_admin(Client *client, int i)
{
    if (Server::channels[i]->isOperator(client->socket_fd))
        return true;
    std::string text;
    text = ":localhost 482 " + client->nickname + " #" + Server::channels[i]->name + " :You're not channel operator\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    return false;
}

bool is_client_can_kick(Client *client, int i)
{
    if (!is_client_joined(client, i))
        return false;
    if (!is_client_admin(client, i))
        return false;
    return true;
}

bool    is_nick_exist(std::string &nick, Client *client, int *j)
{
    for (size_t i = 0; i < Server::array_clients.size(); i++)
    {
        if (Server::array_clients[i]->nickname == nick)
        {
            *j = i;
            return true;
        }
    }
    std::string text;
    text = ":localhost 401 " + client->nickname + " " + nick + " :No such nick/channel\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    return false;
}

bool    is_nick_joined(std::string &nick, int i, int *j, Client *client)
{
    for (size_t k = 0; k < Server::channels[i]->clients.size(); k++)
    {
        if (Server::channels[i]->clients[k]->nickname == nick)
        {
            *j = k;
            return true;
        }
    }
    std::string text;
    text = ":localhost 441 " + client->nickname + " " + nick + " #" + Server::channels[i]->name + " :They aren't on that channel\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    return false;
}

bool valid_nickname(std::string &nick, int i, Client *client, int *j)
{
    int unused = -1;
    if (!is_nick_exist(nick, client, &unused))
        return false;
    if (!is_nick_joined(nick, i, j, client))
        return false;
    return true;
}

void    parse_kick_parameters(std::string &message, std::string &name_channel, std::string &nickname, std::string &reason)
{
    int i = 0;

    i = skip_spaces(message, i);
    if (!message[i])
    {
        message = "";
        return;
    }
    while (message[i] && message[i] != ' ')
    {
        name_channel.push_back(message[i]);
        i++;
    }
    i = skip_spaces(message, i);
    if (!message[i])
    {
        message = "";
        return;
    }
    while (message[i] && message[i] != ' ')
    {
        nickname.push_back(message[i]);
        i++;
    }
    i = skip_spaces(message, i);
    if (!message[i])
        return;
    reason = message.substr(i);
    if (reason[0] == ':')
        reason = reason.substr(1);
}

void remove_invited(Channel *channel)
{
    for (size_t i = 0; i < Server::array_clients.size(); i++)
    {
        un_invite(Server::array_clients[i], channel);
    }
}

void show_operators(Channel *channel)
{
    std::cout << "Operators in channel " << channel->name << ": ";
    for (size_t i = 0; i < channel->operators.size(); i++)
    {
        std::cout << channel->operators[i] << " ";
    }
    std::cout << std::endl;
}

void    inform_all_clients(int i, std::string &text)
{
    for (size_t k = 0; k < Server::channels[i]->clients.size(); k++)
    {
        send(Server::channels[i]->clients[k]->socket_fd, text.c_str(), text.length(), 0);
    }
}

void    kicking(Client *client, int i, int j, std::string &reason)
{
    std::string text;
    Client *kicked_client = Server::channels[i]->clients[j];

    text = ":" + client->nickname + "!" + client->username + "@localhost KICK #" + Server::channels[i]->name + " " + kicked_client->nickname;
    if (!reason.empty())
        text += " :" + reason + "\r\n";
    else
        text += "\r\n";
    inform_all_clients(i, text);
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
    if (Server::channels[i]->clients.size() == 0)
    {
        remove_invited(Server::channels[i]);
        Channel *to_delete = Server::channels[i];
        Server::channels.erase(
            std::remove(Server::channels.begin(),
                        Server::channels.end(),
                        to_delete),
            Server::channels.end());
        delete to_delete;
    }
    else if (Server::channels[i]->isOperator(kicked_client->socket_fd))
    {
        if (Server::channels[i]->operators.size() > 1)
            Server::channels[i]->removeOperator(kicked_client->socket_fd);
        else if (Server::channels[i]->operators.size() == 1)
        {
            Server::channels[i]->removeOperator(kicked_client->socket_fd);
            text = ":localhost MODE #" + Server::channels[i]->name + " +o " + Server::channels[i]->clients[0]->nickname + "\r\n";
            inform_all_clients(i, text);
            Server::channels[i]->addOperator(Server::channels[i]->clients[0]->socket_fd);
        }
    }
}

void    kick(Client *client, std::string &message)
{
    std::string text;
    std::string name_channel;
    std::string nick_to_kick;
    std::string reason;
    int I_CH = -1;
    int I_NK = -1;

    parse_kick_parameters(message, name_channel, nick_to_kick, reason);
    if (message == "")
    {
        text = ":localhost 461 " + client->nickname + " KICK :Not enough parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (!valid_channel(name_channel, client, &I_CH))
        return ;
    if (!is_client_can_kick(client, I_CH))
        return ;
    if (!valid_nickname(nick_to_kick, I_CH, client, &I_NK))
        return ;
    kicking(client, I_CH, I_NK, reason);
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

bool    already_invited(Channel *channel, Client *client)
{
    for (size_t i = 0; i < client->invited_channels.size(); i++)
    {
        if (client->invited_channels[i]->name == channel->name)
            return true;
    }
    return false;
}

void    show_array_clients()
{
    for (unsigned long i = 0; i < Server::array_clients.size(); i++)
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
    for (unsigned long i = 0; i < Server::channels.size(); i++)
    {
        std::cout << "Channel " << i + 1 << ": " << Server::channels[i]->name << std::endl;
        for (unsigned long j = 0; j < Server::channels[i]->clients.size(); j++)
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

void    parse_invite_parameters(std::string &message, std::string &nick_to_invite, std::string &name_channel)
{
    int i = 0;

    i = skip_spaces(message, i);
    if (!message[i])
    {
        message = "";
        return ;
    }
    while (message[i] && message[i] != ' ')
    {
        nick_to_invite.push_back(message[i]);
        i++;
    }
    i = skip_spaces(message, i);
    if (!message[i])
    {
        message = "";
        return ;
    }
    while (message[i] && message[i] != ' ')
    {
        name_channel.push_back(message[i]);
        i++;
    }
}

bool    is_channel_exist(std::string &name_channel, int *I_CH, Client *client)
{
    name_channel = name_channel.substr(1);
    for (size_t i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i]->name == name_channel)
        {
            *I_CH = i;
            return true;
        }
    }
    std::string text;
    text = ":localhost 403 " + client->nickname + " #" + name_channel + " :No such channel\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
    return false;
}

bool    require_admin(Client *client, int I_CH)
{
    if (Server::channels[I_CH]->is_invite_only)
    {
        if (!Server::channels[I_CH]->isOperator(client->socket_fd))
        {
            std::string text;
            text = ":localhost 482 " + client->nickname + " #" + Server::channels[I_CH]->name + " :You're not channel operator\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return true;
        }
    }
    return false;
}

bool    is_already_joined(Client *client, std::string &nick_to_invite, int I_CH)
{
    for (size_t i = 0; i < Server::channels[I_CH]->clients.size(); i++)
    {
        if (Server::channels[I_CH]->clients[i]->nickname == nick_to_invite)
        {
            std::string text;
            text = ":localhost 443 " + client->nickname + " " + nick_to_invite + " #" + Server::channels[I_CH]->name + " :is already on channel\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
            return true;
        }
    }
    return false;
}

void invite(Client *client, std::string &message, std::vector<std::string> &array_params)
{
    std::string text;
    std::string nick_to_invite;
    std::string name_channel;
    int         I_CH = -1;
    int         I_NK = -1;

    if (array_params.size() > 2)
    {
        text = ":localhost 461 " + client->nickname + " INVITE :Too many parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    parse_invite_parameters(message, nick_to_invite, name_channel);
    if (message == "")
    {
        text = ":localhost 461 " + client->nickname + " INVITE :Not enough parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (!is_valid_mask(name_channel, client))
        return ;
    if (!is_channel_exist(name_channel, &I_CH, client))
        return ;
    if (!is_nick_exist(nick_to_invite, client, &I_NK))
        return ;
    if (!is_client_joined(client, I_CH))
        return ;
    if (require_admin(client, I_CH))
        return ;
    if (is_already_joined(client, nick_to_invite, I_CH))
        return ;

    Client  *client_to_invite = Server::array_clients[I_NK];
    Channel *channel = Server::channels[I_CH];

    if (!already_invited(channel, client_to_invite))
        client_to_invite->invited_channels.push_back(channel);

    text = ":" + client->nickname + "!" + client->username + "@localhost INVITE " + nick_to_invite + " #" + channel->name + "\r\n";
    send(Server::array_clients[I_NK]->socket_fd, text.c_str(), text.length(), 0);
    text = ":localhost 341 " + client->nickname + " " + nick_to_invite + " #" + channel->name + "\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
}

void    parse_quit_parameters(std::string &message, std::string &reason)
{
    int i = 0;

    i = skip_spaces(message, i);
    if (!message[i])
        return ;
    reason = message.substr(i);
    if (reason[0] == ':')
        reason = reason.substr(1);
}

void    inform_quit(Client *client, std::string &string)
{
    for (size_t i = 0; i < client->channelsjoined.size(); i++)
    {
        for (size_t j = 0; j < client->channelsjoined[i]->clients.size(); j++)
        {
            if (client->channelsjoined[i]->clients[j]->socket_fd != client->socket_fd)
                send(client->channelsjoined[i]->clients[j]->socket_fd, string.c_str(), string.length(), 0);
        }
    }
    send(client->socket_fd, string.c_str(), string.length(), 0);
}

void    quit(Client *client, std::string message, Server *server, int i)
{
    std::string text;
    std::string reason;

    parse_quit_parameters(message, reason);
    text = ":" + client->nickname + "!" + client->username + "@localhost QUIT";
    if (reason == "")
        text += "\r\n";
    else
        text += " :" + reason + "\r\n";
    inform_quit(client, text);
    close(server->poll_fds[i].fd);
    server->poll_fds.erase(server->poll_fds.begin() + i);
    delete_client(i);
    server->size_cl--;
}

void    unknown_cmd(Client *client, std::string cmd)
{
    std::string text;
    text = ":localhost 421 " + client->nickname + " " + cmd + " :Unknown command\r\n";
    send(client->socket_fd, text.c_str(), text.length(), 0);
}



void    bot_cmd(Client *client, std::string &cmd, Server *server, int *fd_bot)
{
    // std::string bot_response = ":localhost 001 A " + server->handlebotCommand(cmd);
    (void)server;
    std::string bot_request = client->nickname + "::" + cmd;
    int bot_fd = *fd_bot;
    if (bot_fd < 0)
        return ;
    send(bot_fd, bot_request.c_str(), bot_request.length(), 0);
}

void    register_cmd(Client *client, std::string cmd, std::string message, Server *server, int i, int *fd_bot, std::vector<std::string> array_params)
{
    std::string text;

    if (cmd == "PASS" || cmd == "USER")
    {
        text = ":localhost 462 " + client->nickname + " :You may not reregister\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (cmd == "NICK")
        nickname(client, array_params, fd_bot);
    else if (cmd == "JOIN")
        join(client, message, array_params);
    else if (cmd == "KICK")
        kick(client, message);
    else if (cmd == "INVITE")
        invite(client, message, array_params);
    else if(cmd == "QUIT")
        quit(client, message, server, i); // add commands here yosabir
    else if(cmd == "TOPIC")
        topic(client, message, server);
    else if(cmd == "PRIVMSG")
        privmsg(client, message, server);
    else if(cmd == "MODE")
        mode(client, message, server);
    else if (cmd[0] == '!')
        bot_cmd(client, cmd, server, fd_bot);
    else if (cmd == "PONG")
        return ;
    else
        unknown_cmd(client, cmd);
}

std::string command(std::string &message, size_t *index)
{
    size_t j = 0;
    std::string cmd;

    *index = skip_spaces(message, *index);
    message = message.substr(*index);
    j = message.find(" ");
    if (j != std::string::npos)
    {
        cmd = message.substr(0, j);
        message = message.substr(j + 1);
    }
    else
    {
        j = message.find_first_of("\r\n");
        
        if (j != std::string::npos)
            cmd = message.substr(0, j);
        else
            cmd = message;
        message = "";
    }
    j = message.find("\r\n");
    if (j != std::string::npos)
        message = message.substr(0, j);
    return cmd;
}
#include <sstream>

std::vector<std::string> ft_split(std::string &parameters)
{
    std::vector<std::string> result;
    std::string param = "";
    size_t i = 0;

    while (parameters[i])
    {
        i = skip_spaces(parameters, i);
        if (!parameters[i])
            break;
        if (parameters[i] == ':')
        {
            param = parameters.substr(i + 1);
            result.push_back(param);
            break;
        }
        while (parameters[i] && parameters[i] != ' ')
        {
            param.push_back(parameters[i]);
            i++;
        }
        result.push_back(param);
        param = "";
    }
    if (result.size() == 0)
        result.push_back("");
    for (size_t j = 0; j < result.size(); j++)
    {
        std::cout << "Param " << j + 1 << ": [" << result[j] << "]" << std::endl;
    }
    return result;
}

std::string current_time()
{
    std::time_t current_time = std::time(nullptr);
    std::tm* local_time = std::localtime(&current_time);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%a %b %d %Y at %H:%M:%S", local_time);
    return std::string(buffer);
}

bool is_exist_cmd(std::string &cmd)
{
    std::string commands[] = {
        "JOIN", "KICK", "INVITE", "QUIT", "TOPIC",
        "PRIVMSG", "MODE", "PONG"
    };
    for (size_t i = 0; i < commands->length(); i++)
    {
        if (cmd == commands[i])
            return true;
    }
    return false;
}

void    unregister_cmnd(Client *client,std::string &cmd, std::string message, std::string password, int *fd_bot, std::vector<std::string> &array_params)
{
    std::string text;
    std::string NICK = (client->is_nickname) ? client->nickname : "*";
    if (cmd == "BOT")
        client->is_bot = true;
    else if (cmd == "PASS")
        client->is_auth = password_check(client, array_params, password);
    else if (cmd == "NICK")
    {
        if (!client->is_auth)
            return ;
        nickname(client, array_params, fd_bot);
    }
    else if (cmd == "USER")
    {
         if (!client->is_auth)
            return;
        username(client, message);
    }
    else if (is_exist_cmd(cmd))
    {
        text = ":localhost 451 " + NICK + " :You have not registered\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
    }
    else
    {
        text = ":localhost 421 " + NICK + " " + cmd + " :Unknown command\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
    }
    if (client->is_auth && client->is_nickname && client->is_username)
    {
        client->is_registered = true;
        text = ":localhost 001 " + client->nickname + " :Welcome to the IRC Network " + client->nickname + "!" + client->username + "@localhost\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        text = ":localhost 002 " + client->nickname + " :Your host is localhost, running version 1.0\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        text = ":localhost 003 " + client->nickname + " :This server was created " + current_time() + "\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        std::cout << "CLIENT CONNECTED, ON FD: " << client->socket_fd << std::endl;
    }
}

bool    password_check(Client *client, std::vector<std::string> array_params, std::string password)
{
    std::string text;
    std::string NICK = (client->is_nickname) ? client->nickname : "*";
    int socket_fd = client->socket_fd;
    if (array_params.size() > 1)
    {
        text = ":localhost 461 " + NICK + " PASS :Too many parameters\r\n";
        send(socket_fd, text.c_str(), text.length(), 0);
        return false;
    }
    std::string pw = array_params[0];
    if (pw == "")
    {
        text = ":localhost 461 " + NICK + " PASS :Not enough parameters\r\n";
        send(socket_fd, text.c_str(), text.length(), 0);
    }
    else if (password != pw)
    {
        text = ":localhost 464 " + NICK + " :Password incorrect\r\n";
        send(socket_fd, text.c_str(), text.length(), 0);
    }
    if (client->is_bot)//77
    {
        text = ":localhost 001  PASS IS PASS\r\n";
        send(socket_fd, text.c_str(), text.length(), 0);
    }
    return (password == pw);
}

bool    is_new_nickname(std::string nick, int socket_fd, std::string NICK)
{
    std::string text;

    for (unsigned long i = 0; i < Server::array_clients.size(); i++)
    {
        if (Server::array_clients[i]->nickname == nick)
        {
            text = ":localhost 433 " + NICK + " " + nick + " :Nickname is already in use\r\n";
            send(socket_fd, text.c_str(), text.length(), 0);
            return false;
        }
    }
    return true;
}


void    nickname(Client *client, std::vector<std::string> array_params, int *fd_bot)
{
    std::string text;
    std::string NICK = (client->is_nickname) ? client->nickname : "*";
    if (array_params.size() > 1)
    {
        text = ":localhost 461 " + NICK + " NICK :Too many parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    std::string nick = array_params[0];

    if (client->is_bot == false && nick == "[BOT]")//77
    {
        text = ":localhost 432 " + NICK + " " + nick + " :Erroneous nickname (this nickname belongs only to bots)\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    if (nick == "")
    {
        text = ":localhost 431 " + NICK + " :No nickname given\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
    }
    else if (is_illegal(nick))
    {
        text = ":localhost 432 " + NICK + " " + nick + " :Erroneous nickname\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
    }
    else if (is_new_nickname(nick, client->socket_fd, NICK))
    {
        if (client->is_registered)
        {
            std::string text = ":" + client->nickname + "!" + client->username + "@localhost NICK " + nick + "\r\n";
            send(client->socket_fd, text.c_str(), text.length(), 0);
        }
        client->nickname = nick;
        client->is_nickname = true;
    }
    if (client->is_bot)//77
    {
        text = ":localhost 001  nickname is good\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        
        *fd_bot = client->socket_fd;
    }
}


void    show_channels()
{
    for (unsigned long i = 0; i < Server::channels.size(); i++)
    {
        std::cout << "Channel " << i + 1 << ": " << Server::channels[i]->name << std::endl;
        show_operators(Server::channels[i]);
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

void show_clients()
{
    for (int i = 0; i < (int)Server::array_clients.size(); i++)
    {
        std::cout << "Client " << i + 1 << ": " << std::endl;
        std::cout << "Socket FD: " << Server::array_clients[i]->socket_fd << std::endl;
        std::cout << "\tNickname: [" << Server::array_clients[i]->nickname << "]" << std::endl;
        std::cout << "\tUsername: [" << Server::array_clients[i]->username << "]" << std::endl;
        std::cout << "\tReal Name: [" << Server::array_clients[i]->real_name << "]" << std::endl;
        std::cout << "\tIs Authenticated: " << (Server::array_clients[i]->is_auth ? "Yes" : "No") << std::endl;
        std::cout << "\tIs Nickname Set: " << (Server::array_clients[i]->is_nickname ? "Yes" : "No") << std::endl;
        std::cout << "\tIs Username Set: " << (Server::array_clients[i]->is_username ? "Yes" : "No") << std::endl;
        std::cout << "\tIs Registered: " << (Server::array_clients[i]->is_registered ? "Yes" : "No") << std::endl;
        std::cout << "\tIs BOT: " << (Server::array_clients[i]->is_bot ? "Yes" : "No") << std::endl;

        std::cout << "----------------------------" << std::endl;
        std::cout << "\tChannels Joined: ";
        if (Server::array_clients[i]->channelsjoined.empty())
        {
            std::cout << "None" << std::endl;
        }
        else
        {
            std::cout << std::endl;
            for (size_t j = 0; j < Server::array_clients[i]->channelsjoined.size(); j++)
            {
                std::cout << "\t  - [" << Server::array_clients[i]->channelsjoined[j]->name << "]";
                // Channel* ch = Server::array_clients[i]->channelsjoined[j];
                // std::cout << "\t  - " << ch->name;
                // if (!ch->topic.empty())
                //     std::cout << " (Topic: " << ch->topic << ")";
                // else
                //     std::cout << " (No topic set)";
                // std::cout << std::endl;
            }
        }

        std::cout << "-----------------------------" << std::endl;
        std::cout << "\tInvited Channels: ";
        if (Server::array_clients[i]->invited_channels.empty())
            std::cout << "None" << std::endl;
        else
        {
            for (size_t j = 0; j < Server::array_clients[i]->invited_channels.size(); j++)
            {
                std::cout << Server::array_clients[i]->invited_channels[j]->name << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------------------------" << std::endl;
    }

    show_channels();
    std::cout << "\e[1;31m*****************************************************\e[0m" << std::endl;
    std::cout << "\e[1;31m*****************************************************\e[0m" << std::endl;
    std::cout << "\e[1;31m*****************************************************\e[0m" << std::endl;
}

size_t skip_spaces(std::string message, size_t i)
{
    while (message[i] && message[i] == ' ')
        i++;
    return i;
}

void split_to_four(std::string message, std::vector<std::string> &array_string)
{
    size_t i = skip_spaces(message, 0);
    size_t start = i;
    std::string token;
    int num = 0;
    while (message[i])
    {
        if ((message[i] == ' ') && num < 3)
        {
            token = message.substr(start, i - start);
            array_string.push_back(token);
            i = skip_spaces(message, i);
            start = i;
            num++;
        }
        if (message[i] && num == 3)
        {
            while (message[i])
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

bool is_valid_user(std::vector<std::string> &array_string, Client *client, std::string NICK)
{
    std::string text;

    if (array_string.size() != 4)
    {
        text = ":localhost 461 " + NICK + " USER :Not enough parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return false;
    }
    return true;
}

std::string real_name(std::string &real)
{
    if (real[0] == ':')
        return real.substr(1);
    return real;
}

void    username(Client *client, std::string message)
{
    std::vector<std::string> array_string;
    std::string text;
    std::string NICK = (client->is_nickname) ? client->nickname : "*";
    if (message == "")
    {
        text = ":localhost 461 " + NICK + " USER :Not enough parameters\r\n";
        send(client->socket_fd, text.c_str(), text.length(), 0);
        return ;
    }
    split_to_four(message, array_string);
    if (is_valid_user(array_string, client, NICK))
    {
        client->username = array_string[0];
        client->real_name = real_name(array_string[3]);
        client->is_username = true;
    }
    
}

bool channel_has_one_user(Channel *channel)
{
    if (channel->clients.size() == 1)
        return true;
    return false;
}

void    remove_channel(Channel *channel)
{
    for (unsigned long i = 0; i < Server::channels.size(); i++)
    {
        if (Server::channels[i]->name == channel->name)
        {
            Server::channels.erase(
                std::remove(Server::channels.begin(), Server::channels.end(), Server::channels[i]),
                Server::channels.end());
            remove_invited(channel);
            channel->clients.clear();
            delete channel;
            return ;
        }
    }
}

void    handle_channels(int i)
{
    for (unsigned long j = 0; j < Server::array_clients[i - 1]->channelsjoined.size(); j++)
    {
        std::cout << "Handling channel: " << Server::array_clients[i - 1]->channelsjoined[j]->name << std::endl;
        Channel *channel = Server::array_clients[i - 1]->channelsjoined[j];
        if (channel_has_one_user(channel))
        {
            remove_channel(channel);
            Server::array_clients[i - 1]->channelsjoined.clear();
            Server::array_clients[i - 1]->invited_channels.clear();
            return ;
        }
        channel->clients.erase(
            std::remove(channel->clients.begin(), channel->clients.end(), Server::array_clients[i - 1]),
            channel->clients.end());
        if (channel->isOperator(Server::array_clients[i - 1]->socket_fd))
        {
            if (channel->operators.size() > 1)
                channel->removeOperator(Server::array_clients[i - 1]->socket_fd);
            else if (channel->operators.size() == 1)
            {
                channel->removeOperator(Server::array_clients[i - 1]->socket_fd);
                std::string text = ":localhost MODE #" + channel->name + " +o " + channel->clients[0]->nickname + "\r\n";
                inform_all_clients(j, text);
                channel->addOperator(channel->clients[0]->socket_fd);
            }
        }
    }
    Server::array_clients[i - 1]->channelsjoined.clear();
    Server::array_clients[i - 1]->invited_channels.clear();
}

void    delete_client(int i)
{
    Client *delet_client = Server::array_clients[i - 1];
    handle_channels(i);
    Server::array_clients.erase(
        std::remove(Server::array_clients.begin(), Server::array_clients.end(), Server::array_clients[i - 1]),
        Server::array_clients.end());
    delete delet_client;
}

void    Server::execute(Client *client, std::string &message, int i,int *fd_bot)
{
    (void)i;
    size_t  index = 0;
    std::string cmd = command(message, &index);
    std::vector<std::string> array_params = ft_split(message);
    if (cmd == "" && index == 0)
        return ;
    if (!client->is_registered)
        unregister_cmnd(client, cmd, message, this->pw, fd_bot, array_params);//done
    else
        register_cmd(client, cmd, message, this, i, fd_bot, array_params);
}

void Server::run()
{
    int client_fd;
    char buf[513];
    memset(buf, 0, 513);
    size_cl = 0;
    struct sockaddr_in cl_adr;
    size_t bytes_readen;
    socklen_t cl_len = sizeof(cl_adr);
    int i;
    int socket_bot = -1;
    while (true)
    {
        int ready = poll(this->poll_fds.data(), this->poll_fds.size(), 1);
        if (ready < 0)
        {
            std::cerr << "problem with poll"<<std::endl;
            continue;
        }
        if (this->poll_fds.at(0).revents & POLLIN)
        {
            client_fd = accept(this->get_serv_fd(), (struct sockaddr *)&cl_adr, &cl_len);
            if (client_fd < 0)
            {
                std::cerr << "Error while accepting new client\n" << std::endl;
                continue ;
            }
            size_cl++;
            std::cout << "client of socket <" << client_fd << "> CONNECTED" << std::endl;
            register_cl(&this->poll_fds, client_fd);
            array_clients.push_back(new Client(client_fd));
            std::string buf_client = "";
            buffs.push_back(buf_client);
        }
        i = 0;
        while (++i <= size_cl)
        {
            if (this->poll_fds.at(i).revents & POLLIN)
            {
                memset(buf, 0, 513);
                bytes_readen = recv(this->poll_fds.at(i).fd, &buf, sizeof(buf), 0);
                if (bytes_readen > 0)
                {
                    std::string str(buf);
                    buffs.at(i - 1) += str;
                    std::cout << "Message from client " << i << ": " << buffs.at(i - 1) << std::endl;
                    std::string cur;
                    unsigned long pos = 0;
                    while ((pos = buffs.at(i - 1).find_first_of("\n")) != (unsigned long) std::string::npos)
                    {
                        if (buffs.at(i - 1)[pos - 1] == '\r')
                            cur = buffs.at(i - 1).substr(0, pos - 1);
                        else
                            cur = buffs.at(i - 1).substr(0, pos);
                        execute(array_clients.at(i - 1), cur, i, &socket_bot);
                        if (pos > 0 && buffs[i-1][pos - 1] == '\r')
                        {
                            cur = buffs[i-1].substr(0, pos - 1);
                            buffs[i-1] = buffs[i-1].substr(pos + 1);
                        }
                        else
                        {
                            cur = buffs[i-1].substr(0, pos);
                            buffs[i-1] = buffs[i-1].substr(pos + 1);
                        }
                        show_clients();
                    }
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
