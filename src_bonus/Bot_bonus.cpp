#include <netdb.h> // getaddrinfo
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include "../include_bonus/bot_bonus.hpp"


Bot::Bot(std::string port, std::string password)
{
    this->port = port;
    this->pw = password;
}


int Bot::check()
{
    if (port.size() == 0 || pw.size() == 0)
        return 1;
}



int Bot::setup()
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res;

    if (getaddrinfo("127.0.0.1", this->port.c_str(), &hints, &res))
        return (1);

    this->fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (this->fd < 0)
        return (1);
    if (connect(this->fd, res->ai_addr, res->ai_addrlen))
    {
        close(this->fd);
        return 1;
    }
    freeaddrinfo(res);
    return 0;
}

int auth(std::string pw,int fd)
{
    std::string pass = "PASS " + pw + "\r\n";
    std::string nick = "NICK [BOT]\r\n";
    std::string user = "USER [BOT] [BOT] [BOT] [BOT]\r\n";


   
    send(fd, pass.c_str(), pass.length(), 0);
    char rec[1000];

    unsigned int n = recv(fd, rec, sizeof(rec), 0);
    if (n <= 0)
        return 1;
    rec[n] = '\0';
    std::string msg_pw(rec);
    if (msg_pw.find(" 464 ") != std::string::npos)
    {
        std::cerr << "Password typed is incorrect, closing connection" <<std::endl;
        return 1;
    }

    send(fd, nick.c_str(), nick.length(), 0);
    memset(rec, 0, 1000);
    n = recv(fd, rec, sizeof(rec), 0);
        if (n <= 0) 
            return 1;
    rec[n] = '\0';
    std::string msg_nick(rec);
    if (msg_nick.find(" 432 ") != std::string::npos)
    {
        std::cerr << "Nickname typed is incorrect, closing connection" <<std::endl;
        return 1;
    }

    send(fd, user.c_str(), user.length(), 0);
    memset(rec, 0, 1000);
    n = recv(fd, rec, sizeof(rec) - 1, 0);
        if (n <= 0) 
            return 1;
    rec[n] = '\0';
    std::string msg_us(rec);
    if (msg_us.find(" 461 ") != std::string::npos)
    {
        std::cerr << "parameters of USER cmd are incorrect, closing connection" <<std::endl;
        return 1;
    }
    return 0;
}

void Bot::run()
{
    if (auth(this->pw, this->fd))
        return ;
    while (1)
    {
        char buf[1024];
        ssize_t n = recv(this->fd, buf, sizeof(buf)-1, 0);
        if (n <= 0)
        {
            std::cerr << "connexion closed. quitting program" << std::endl;
            return ;
        }

        buf[n] = '\0';
        std::string msg(buf);
        int sep = msg.find("::");
        if (sep == std::string::npos)
        {
            std::cerr<<"Invalid format." <<std::endl;
            continue ;
        }
        std::string nick = msg.substr(0, sep);
        std::string command = msg.substr(sep + 2);
        if (nick.size() == 0 || command.size() == 0)
        {
            std::cerr<<"Invalid format." <<std::endl;
            continue ;
        }
        std::string response = "PRIVMSG " + nick + " :Command received: processing: " + command + "\r\n";
        send(this->fd, response.c_str(), response.size(), 0);
        exit(1);
    }    
}