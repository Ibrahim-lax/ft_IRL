#include <netdb.h> // getaddrinfo
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <vector>
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
    return 0;
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
    {
        freeaddrinfo(res);
        std::cerr << "socket error"<<std::endl;
        return (1);
    }
    if (connect(this->fd, res->ai_addr, res->ai_addrlen))
    {
        close(this->fd);
        std::cerr << "connect() error"<<std::endl;
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res);
    this->bot_start_time = time(0);
    return 0;
}

int auth(std::string pw,int fd)
{
    std::string flag = "BOT\r\n";
    std::string pass = "PASS " + pw + "\r\n";
    std::string nick = "NICK [BOT]\r\n";
    std::string user = "USER [BOT] [BOT] [BOT] [BOT]\r\n";

    send(fd, flag.c_str(), flag.length(), 0);
    usleep(500);
    send(fd, pass.c_str(), pass.length(), 0);
    usleep(500);
    char rec[1000];
    ssize_t n = recv(fd, rec, sizeof(rec), 0);
    if (n > 0)
    {
        std::string msg_pw(rec, n);
        if (msg_pw.size() && msg_pw.find("464") != std::string::npos)
        {
            std::cerr << "Password typed is incorrect, closing connection" <<std::endl;
            return 1;
        }
        else if (msg_pw.size() && msg_pw.find("001") != std::string::npos)
            std::cerr << "Password typed is correct" <<std::endl;
    }
    send(fd, nick.c_str(), nick.length(), 0);
    usleep(500);
    memset(rec, 0, 1000);
    n = recv(fd, rec, sizeof(rec), 0);
    if (n > 0)
    {
        std::string msg_nick(rec, n);
        if (msg_nick.size() && msg_nick.find(" 432 ") != std::string::npos)
        {
            std::cerr << "Nickname typed is incorrect, closing connection" <<std::endl;
            return 1;
        }
        else if (msg_nick.size() && msg_nick.find("001") != std::string::npos)
            std::cerr << "Nickname typed is correct" <<std::endl;
    }
    send(fd, user.c_str(), user.length(), 0);
    usleep(500);
    memset(rec, 0, 1000);
    n = recv(fd, rec, sizeof(rec), 0);
    if (n > 0)
    {
        std::string msg_us(rec, n);
        if (msg_us.size() && msg_us.find(" 461 ") != std::string::npos)
        {
            std::cerr << "Parameters of USER cmd are incorrect, closing connection" <<std::endl;
            return 1;
        }
        else if (msg_us.size() && msg_us.find("001") != std::string::npos)
            std::cerr << "Username typed is correct" <<std::endl;
    }
    return 0;
}

std::string handlebotCommand(std::string &cmd, long t, int fd, std::string nickname)
{
    std::vector<std::string> jokes;
    jokes.push_back("Why do programmers prefer dark mode? Because light attracts bugs!");
    jokes.push_back("Why did the function return early? It had too many arguments!");
    jokes.push_back("I would tell you a UDP joke, but you might not get it.");
    jokes.push_back("if your code works, dont touch it");


    if (!cmd.find("!uptime") || !cmd.find("!UPTIME"))
    {
        time_t now = time(0);
        long uptime_sec = now - t;
        return "Bot started " + std::to_string(uptime_sec) + " seconds ago";
    } 
    else if (!cmd.find("!JOKE") || !cmd.find("!joke"))
    {
        int idx = rand() % jokes.size();
        return jokes[idx];
    } 
    else if (!cmd.find("!HELP") || !cmd.find("!help"))
    {
        std::string snd_res;
        //"Bot: Available commands:\n     !uptime - shows server uptime in seconds\n     !joke - tells a random joke\n"
        snd_res = "PRIVMSG " + nickname + " Available commands :\r\n";
        send(fd, snd_res.c_str(), snd_res.size(), 0);
        snd_res = "PRIVMSG " + nickname + "     !uptime - shows server uptime in seconds\r\n";
        send(fd, snd_res.c_str(), snd_res.size(), 0);
        snd_res = "PRIVMSG " + nickname + "     !joke - tells a random joke\r\n";
        send(fd, snd_res.c_str(), snd_res.size(), 0);
        return "     !help - shows this help message";
    }
    return "Bot: Unknown command. Try !help";
}

void Bot::run()
{
    if (auth(this->pw, this->fd))
    {
        close(this->fd);
        return ;
    }
    std::cout << "AUTHENTICATION DONE"<<std::endl;
    std::cout << "MY SOCKET : " << this->fd << std::endl;

    ssize_t n;
    while (1)
    {
        char buf[1024];
        n = recv(this->fd, buf, sizeof(buf)-1, 0);
        if (n == 0)
        {
            std::cout << "Connexion is closed quiting ..."<<std::endl;
            break ;
        }
        if (n)
        {
            buf[n] = '\0';
            std::string msg(buf);
            unsigned long sep = msg.find("::");
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
            std::string m = handlebotCommand(command, this->bot_start_time,this->fd, nick);
            std::string response = "PRIVMSG " + nick + " " + m + "\r\n";
            send(this->fd, response.c_str(), response.size(), 0);
        }
    }    
}


