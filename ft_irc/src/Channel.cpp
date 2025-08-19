#include "../include/Channel.hpp"




void sendMessage(const std::string &msg) 
{
        const char *data = msg.c_str();
        size_t total = 0;
        size_t len = msg.length();

        while (total < len) 
        {
            ssize_t sent = send(socket_fd, data + total, len - total, 0);
            if (sent <= 0) 
            {
                close(socket_fd);
                socket_fd = -1;
                break;
            }
            total += sent;
        }
}


void broadcast(const std::string &msg, Client *except = NULL) 
{
        for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
        {
            if (*it != except) 
            {
                (*it)->sendMessage(msg);
            }
        }
}

void handlePrivmsg(Client &client, const std::vector<std::string> &args,
                   std::map<std::string, Channel*> &channels,
                   std::map<std::string, Client*> &clients) 
    {
    if (args.size() < 2) 
    {
        client.sendMessage(":Not enough parameters\r\n");
        return;
    }
    std::string target = args[0];
    std::string message = args[1];

    if (target[0] == '#') 
    {
        std::map<std::string, Channel*>::iterator it = channels.find(target);
        if (it == channels.end()) 
        {
            client.sendMessage("No such channel\r\n");
            return;
        }
        it->second->broadcast(":" + client.getFullName() + " PRIVMSG " + target + " :" + message + "\r\n", &client);
    }
    
    else 
    {
        std::map<std::string, Client*>::iterator it = clients.find(target);
        if (it == clients.end()) 
        {
            client.sendMessage(target + " :No such nick\r\n");
            return;
        }
        it->second->sendMessage(":" + client.nickname + " PRIVMSG " + target + " :" + message + "\r\n");
    }
}

void handleTopic(Client &client, const std::vector<std::string> &args,
                 std::map<std::string, Channel*> &channels) 
{
    if (args.size() < 1)
        return;
    std::string chanName = args[0];

    std::map<std::string, Channel*>::iterator it = channels.find(chanName);
    if (it == channels.end()) 
    {
        client.sendMessage(chanName + " :No such channel\r\n");
        return;
    }
    Channel *chan = it->second;

    if (args.size() == 1) 
    {
        if (chan->getTopic().empty())
            client.sendMessage(" chanName "+ " :No topic is set\r\n");
        else
            client.sendMessage(" chanName " +  ": " + chan->getTopic() + "\r\n");
    } 
    else 
    { 
        if (!chan->isOperator(client)) 
        {
            client.sendMessage("482 " + chanName + " :You're not channel operator\r\n");
            return;
        }
        chan->setTopic(args[1]);
        chan->broadcast(":" + client.getFullName() + " TOPIC " + chanName + " :" + args[1] + "\r\n");
    }
}


void Channel::setInviteOnly(bool add)
{
    if (add)
        channel->invite_only = 1;
}

void handleMode(Client &client, const std::vector<std::string> &args,
                std::map<std::string, Channel*> &channels) 
{
    if (args.size() < 2) return;
    std::string chanName = args[0];
    std::string modeStr = args[1];

    std::map<std::string, Channel*>::iterator it = channels.find(chanName);
    if (it == channels.end()) 
    {
        client.sendMessage("No such channel\r\n");
           return;
    }
    Channel *chan = it->second;

    if (!chan->isOperator(client)) 
    {
        client.sendMessage("You're not channel operator\r\n");
        return;
    }

    bool add = (modeStr[0] == '+');
    char mode = modeStr[1];

    switch (mode) 
    {
         case 'i':chan->setInviteOnly(add); break;
         case 't': chan->setTopicRestrict(add); break;
        /case 'k': 
        // case 'o':
        // case 'l':
    }

    chan->broadcast(":" + client.getFullName() + " MODE " + chanName + " " + modeStr + "\r\n");
}

void setOperator(std::map<std::string, Client*> &clients, const std::string &nick, bool opp) 
{
    std::map<std::string, Client*>::iterator it = clients.find(nick);
    if (it == clients.end()) 
    {
        std::cout << nick << " :No such nick\r\n";
        return;
    }

    Client* client = it->second;

    if (client->is_admin == 1)
        return;

    client->is_admin = opp ? 1 : 0;
}
