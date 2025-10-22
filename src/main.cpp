/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 17:12:48 by librahim          #+#    #+#             */
/*   Updated: 2025/10/22 18:22:41 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

// int main(int ac, char *av[])
// {
//     signal(SIGPIPE, SIG_IGN);

//     Server s("6667", "0");
//     s.setup();
//     s.run();
//     return 0;
// }
// void l() {system("leaks Irc_server");}
bool isNumeric(const std::string &str)
{
    if (str.empty())
        return false;

    size_t i = 0;
    if (str[0] == '-') // skip minus sign
    {
        if (str.size() == 1) // only "-" is invalid
            return false;
        i = 1;
    }

    for (; i < str.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(str[i])))
            return false;
    }

    return true;
}

void clean_server()
{
    for (size_t i = 0; i < Server::channels.size(); i++)
    {
        delete Server::channels[i];
    }
    for (size_t i = 0; i < Server::array_clients.size(); i++)
    {
        delete Server::array_clients[i];
    }
    for (size_t i = 0; i < Server::poll_fds.size(); i++)
    {
        if (i == 0)
            continue;
        close(Server::poll_fds[i].fd);
    }
    Server::channels.clear();
    Server::array_clients.clear();
    Server::poll_fds.clear();
}


void handler(int signum)
{
    (void)signum;
    if (Server::is_server_running)
        clean_server();
    exit(0);
}


int main(int ac, char *av[])
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handler);

    if (ac < 3) {
        std::cerr << "Usage: " << av[0] << " <port> <password>\n";
        return 1;
    }

    std::string port = av[1];
    if (port.size() == 0)
    {
        std::cerr << "error : <port> cant be empty" << std::endl;
        return 1;
    }
    if (isNumeric(port) == false)
    {
        std::cerr << "error : <port> is not numeric" << std::endl;
        return 1;
    }
    std::string pw = av[2];
    if (pw.size() == 0)
    {
        std::cerr << "error : <password> cant be empty" << std::endl;
        return 1;
    }
    long long port_num = atol(av[1]);
    if (port_num < 1024 || port_num > 49100)
    {
        std::cerr << "error : <port> is out of range" << std::endl;
        return 1;
    }
    Server s(av[1], av[2]);
    if (pw.size() == 0)
        s.password_empty = true;
    else
        s.password_empty = false;
    s.setup();
    s.run();
    
    return 0;
}