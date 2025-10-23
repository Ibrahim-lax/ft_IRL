#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include "../include_bonus/bot_bonus.hpp"



int main(int ac, char **v)
{
    if (ac != 3)
    {
        std::cerr << "Usage : ./BOT <PORT> <PASSWORD>" <<std::endl;
        return 1;
    }
    Bot bot(v[1], v[2]);
    if (bot.check())
    {
        std::cerr << "Error in input" << std::endl;
        return 1;
    }
    if (bot.setup())
        return 1;
    bot.run();
    return 0;
}