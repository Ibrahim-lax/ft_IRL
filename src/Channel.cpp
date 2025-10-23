/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/01 15:18:31 by yosabir           #+#    #+#             */
/*   Updated: 2025/10/23 16:48:55 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include <sstream>


void topic(Client *client, std::string &message, Server *server)
{
    size_t i = 0;
    while (i < message.size() && message[i] == ' ') 
        i++;

    std::string chanName;
    while (i < message.size() && message[i] != ' ')
    {
        chanName += message[i];
        i++;
    }

    while (i < message.size() && message[i] == ' ') 
        i++;

    std::string topicText;
    if (i < message.size() && message[i] == ':')
    {
        i++;
        topicText = message.substr(i);
    }

    if (!chanName.empty() && chanName[0] == '#')
        chanName = chanName.substr(1);

    Channel *channel = NULL;
    for (size_t j = 0; j < server->channels.size(); j++)
    {
        if (server->channels[j]->name == chanName)
        {
            channel = server->channels[j];
            break;
        }
    }

    if (!channel)
    {
        std::string reply = "403 " + chanName + " :No such channel\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    bool inChannel = false;
    for (size_t j = 0; j < channel->clients.size(); j++)
    {
        if (channel->clients[j] == client)
        {
            inChannel = true;
            break;
        }
    }

    if (!inChannel)
    {
        std::string reply = "442 #" + channel->name + " :You're not on that channel\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    if (topicText.empty())
    {
        std::string reply = channel->topic.empty()
            ? "331 #" + channel->name + " :No topic is set\r\n"
            : "332 #" + channel->name + " :" + channel->topic + "\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    if (channel->is_topic_restricted && !channel->isOperator(client->socket_fd))
    {
        std::string reply = "482 #" + channel->name + " :You're not channel operator\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    channel->topic = topicText;
    channel->topic_join_msg = true;

    std::string notify = ":" + client->nickname + " TOPIC #" + channel->name + " :" + topicText + "\r\n";
    for (size_t j = 0; j < channel->clients.size(); j++)
    {
        send(channel->clients[j]->socket_fd, notify.c_str(), notify.length(), 0);
    }
}

void privmsg(Client *client, std::string &message, Server *server)
{
    size_t i = 0;

    while (i < message.size() && message[i] == ' ')
        i++;

    std::string targets;
    while (i < message.size() && message[i] != ' ')
        targets += message[i++];

    while (i < message.size() && message[i] == ' ')
        i++;

    std::string msgText;
    if (i < message.size())
    {
        msgText = message.substr(i);
        if (!msgText.empty() && msgText[0] == ':')
            msgText.erase(0, 1);
    }

    if (targets.empty())
    {
        std::string reply = "411 " + client->nickname + " :No recipient given (PRIVMSG)\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    if (msgText.empty())
    {
        std::string reply = "412 " + client->nickname + " :No text to send\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    // detect CTCP/DCC message ASCII 1
    bool is_ctcp = false;
    if (msgText.size() > 2 && msgText[0] == '\x01' && msgText[msgText.size() - 1] == '\x01')
        is_ctcp = true;

    std::string::size_type start = 0, end;
    while ((end = targets.find(',', start)) != std::string::npos || start < targets.size())
    {
        std::string target = (end == std::string::npos) ? targets.substr(start)
                                                        : targets.substr(start, end - start);
        start = (end == std::string::npos) ? targets.size() : end + 1;
        if (target.empty())
            continue;

        if (target[0] == '#')
        {
            Channel *channel = NULL;
            for (size_t j = 0; j < server->channels.size(); j++)
            {
                if (server->channels[j]->name == target.substr(1))
                {
                    channel = server->channels[j];
                    break;
                }
            }

            if (!channel)
            {
                std::string reply = "403 " + client->nickname + " " + target + " :No such channel\r\n";
                send(client->socket_fd, reply.c_str(), reply.length(), 0);
                continue;
            }

            bool inChannel = false;
            for (size_t j = 0; j < channel->clients.size(); j++)
            {
                if (channel->clients[j] == client)
                {
                    inChannel = true;
                    break;
                }
            }

            if (!inChannel)
            {
                std::string reply = "442 " + client->nickname + " " + target + " :You're not on that channel\r\n";
                send(client->socket_fd, reply.c_str(), reply.length(), 0);
                continue;
            }

            std::string notify = ":" + client->nickname + "!" + client->username + "@localhost PRIVMSG " + target + " :" + msgText + "\r\n";
            for (size_t j = 0; j < channel->clients.size(); j++)
            {
                if (channel->clients[j] != client)
                    send(channel->clients[j]->socket_fd, notify.c_str(), notify.length(), 0);
            }
        }
        else
        {
            Client *receiver = NULL;
            for (size_t j = 0; j < server->array_clients.size(); j++)
            {
                if (server->array_clients[j]->nickname == target)
                {
                    receiver = server->array_clients[j];
                    break;
                }
            }

            if (!receiver)
            {
                std::string reply = "401 " + client->nickname + " " + target + " :No such nick\r\n";
                send(client->socket_fd, reply.c_str(), reply.length(), 0);
                continue;
            }

            // Send CTCP (DCC)
            std::string notify = ":" + client->nickname + "!" + client->username + "@localhost PRIVMSG " + target + " :";
            if (is_ctcp)
                notify += msgText;
            else
                notify += msgText;
            notify += "\r\n";

            send(receiver->socket_fd, notify.c_str(), notify.length(), 0);
        }

        if (end == std::string::npos)
            break;
    }
}




void mode(Client *client, std::string &message, Server *server)
{
    size_t i = 0;
    while (i < message.size() && message[i] == ' ')
        i++;

    std::string chanName;
    while (i < message.size() && message[i] != ' ')
    {
        chanName += message[i];
        i++;
    }

    while (i < message.size() && message[i] == ' ')
        i++;

    std::string modeStr;
    while (i < message.size() && message[i] != ' ')
    {
        modeStr += message[i];
        i++;
    }

    while (i < message.size() && message[i] == ' ')
        i++;

    std::string modeArg;
    if (i < message.size())
        modeArg = message.substr(i);

    if (!chanName.empty() && chanName[0] == '#')
        chanName = chanName.substr(1);

    Channel *channel = NULL;
    for (size_t j = 0; j < server->channels.size(); j++)
    {
        if (server->channels[j]->name == chanName)
        {
            channel = server->channels[j];
            break;
        }
    }

    if (!channel)
    {
        std::string reply = "403 " + chanName + " :No such channel\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    bool inChannel = false;
    for (size_t j = 0; j < channel->clients.size(); j++)
    {
        if (channel->clients[j] == client)
        {
            inChannel = true;
            break;
        }
    }

    if (!inChannel)
    {
        std::string reply = "442 #" + channel->name + " :You're not on that channel\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    if (modeStr.empty())
    {
        std::string modes = "+";
        if (channel->is_invite_only) modes += "i";
        if (channel->is_topic_restricted) modes += "t";
        if (channel->is_password_required) modes += "k";
        if (channel->is_limit_set) modes += "l";

        std::string reply = "324 " + client->nickname + " #" + channel->name + " " + modes + "\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    if (!channel->isOperator(client->socket_fd))
    {
        std::string reply = "482 #" + channel->name + " :You're not channel operator\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }
    bool add = true;
    for (size_t j = 0; j < modeStr.size(); j++)
    {
        char c = modeStr[j];
        if (c == '+') { add = true; continue; }
        if (c == '-') { add = false; continue; }

        switch (c)
        {
            case 'i':
                channel->is_invite_only = add;
                break;
            case 't':
                channel->is_topic_restricted = add;
                break;
            case 'k':
                if (add)
                {
                    if (!modeArg.empty())
                    {
                        channel->is_password_required = true;
                        channel->password = modeArg;
                    }
                }
                else
                {
                    channel->is_password_required = false;
                    channel->password = "";
                }
                break;
            case 'o':
                if (!modeArg.empty())
                {
                    Client *target = NULL;
                    for (size_t k = 0; k < channel->clients.size(); k++)
                    {
                        if (channel->clients[k]->nickname == modeArg)
                        {
                            target = channel->clients[k];
                            break;
                        }
                    }

                    if (target)
                    {
                        if (add)
                            channel->addOperator(target->socket_fd);
                        else
                            channel->removeOperator(target->socket_fd);
                    }
                }
                break;
            case 'l':
                if (add)
                {
                    if (!modeArg.empty())
                    {
                        int lim = atoi(modeArg.c_str());
                        channel->limit = lim;
                        channel->is_limit_set = true;
                    }
                }
                else
                {
                    channel->is_limit_set = false;
                }
                break;
            default:
            {
                std::string reply = ": 472 " + client->nickname +
                            " " + c + " :Unknown MODE flag\r\n";
                send(client->socket_fd, reply.c_str(), reply.length(), 0);
                break;
            }
        }
    }

    std::string notify = ":" + client->nickname + " MODE #" + channel->name + " " + modeStr;
    if (!modeArg.empty())
        notify += " " + modeArg;
    notify += "\r\n";

    for (size_t j = 0; j < channel->clients.size(); j++)
        send(channel->clients[j]->socket_fd, notify.c_str(), notify.length(), 0);
}

