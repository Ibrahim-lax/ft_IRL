/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosabir <yosabir@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/01 15:18:31 by yosabir           #+#    #+#             */
/*   Updated: 2025/09/17 10:18:37 by yosabir          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

void topic(Client *client, std::string &message, Server *server)
{
    size_t i = 0;
    while (i < message.size() && message[i] == ' ') i++; // skip spaces

    // extract channel
    std::string chanName;
    while (i < message.size() && message[i] != ' ')
    {
        chanName += message[i];
        i++;
    }

    while (i < message.size() && message[i] == ' ') i++; // skip spaces

    // extract topic text (optional)
    std::string topicText;
    if (i < message.size() && message[i] == ':')
    {
        i++; // skip ':'
        topicText = message.substr(i); // everything after ':' is topic
    }

    // remove '#' if present
    if (!chanName.empty() && chanName[0] == '#')
        chanName = chanName.substr(1);

    // find channel
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

    // check if client is in channel
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

    // if topic is empty → send current topic
    if (topicText.empty())
    {
        std::string reply = channel->topic.empty()
            ? "331 #" + channel->name + " :No topic is set\r\n"
            : "332 #" + channel->name + " :" + channel->topic + "\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    // if topic restricted, check if client is an operator
    if (channel->is_topic_restricted && !channel->isOperator(client->socket_fd))
    {
        std::string reply = "482 #" + channel->name + " :You're not channel operator\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    // set new topic
    channel->topic = topicText;

    // broadcast to all clients in channel
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
        i++; // skip spaces

    // extract target (channel or nickname)
    std::string target;
    while (i < message.size() && message[i] != ' ')
    {
        target += message[i];
        i++;
    }

    while (i < message.size() && message[i] == ' ')
        i++; // skip spaces

    // extract message text
    std::string msgText;
    if (i < message.size() && message[i] == ':')
    {
        i++; // skip ':'
        msgText = message.substr(i);
    }

    // if no message → error
    if (msgText.empty())
    {
        std::string reply = "412 " + client->nickname + " :No text to send\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    // PRIVMSG to channel
    if (!target.empty() && target[0] == '#')
    {
        std::string chanName = target.substr(1); // remove '#'
        Channel *channel = NULL;

        // find channel
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
            std::string reply = "403 " + target + " :No such channel\r\n";
            send(client->socket_fd, reply.c_str(), reply.length(), 0);
            return;
        }

        // check if sender is in channel
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
            std::string reply = "442 " + target + " :You're not on that channel\r\n";
            send(client->socket_fd, reply.c_str(), reply.length(), 0);
            return;
        }

        // build broadcast message
        std::string notify = ":" + client->nickname + " PRIVMSG " + target + " :" + msgText + "\r\n";

        // send to all except sender
        for (size_t j = 0; j < channel->clients.size(); j++)
        {
            if (channel->clients[j] != client)
                send(channel->clients[j]->socket_fd, notify.c_str(), notify.length(), 0);
        }
    }
    else // PRIVMSG to nickname
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
            std::string reply = "401 " + target + " :No such nick\r\n";
            send(client->socket_fd, reply.c_str(), reply.length(), 0);
            return;
        }

        // build direct message
        std::string notify = ":" + client->nickname + " PRIVMSG " + target + " :" + msgText + "\r\n";
        send(receiver->socket_fd, notify.c_str(), notify.length(), 0);
    }
}

void mode(Client *client, std::string &message, Server *server)
{
    size_t i = 0;
    while (i < message.size() && message[i] == ' ')
        i++;

    // extract channel
    std::string chanName;
    while (i < message.size() && message[i] != ' ')
    {
        chanName += message[i];
        i++;
    }

    while (i < message.size() && message[i] == ' ')
        i++;

    // extract mode string
    std::string modeStr;
    while (i < message.size() && message[i] != ' ')
    {
        modeStr += message[i];
        i++;
    }

    while (i < message.size() && message[i] == ' ')
        i++;

    // optional argument (password, limit, target nick for +o/-o)
    std::string modeArg;
    if (i < message.size())
        modeArg = message.substr(i);

    // remove '#' if present
    if (!chanName.empty() && chanName[0] == '#')
        chanName = chanName.substr(1);

    // find channel
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

    // check if client is in channel
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

    // if no mode string → show current modes
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

    // only operator can change modes
    if (!channel->isOperator(client->socket_fd))
    {
        std::string reply = "482 #" + channel->name + " :You're not channel operator\r\n";
        send(client->socket_fd, reply.c_str(), reply.length(), 0);
        return;
    }

    // apply modes
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
                            channel->removeOperator(target->socket_fd); // creator never removed
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
        }
    }

    // broadcast mode change
    std::string notify = ":" + client->nickname + " MODE #" + channel->name + " " + modeStr;
    if (!modeArg.empty())
        notify += " " + modeArg;
    notify += "\r\n";

    for (size_t j = 0; j < channel->clients.size(); j++)
        send(channel->clients[j]->socket_fd, notify.c_str(), notify.length(), 0);
}

