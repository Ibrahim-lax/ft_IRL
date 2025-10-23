/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 17:12:48 by librahim          #+#    #+#             */
/*   Updated: 2025/10/23 15:12:26 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

bool isnum(std::string str)
{
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (!std::isdigit((str[i])))
            return false;
    }
    return true;
}

void Server::clean_server()
{
    for (size_t i = 0; i < channels.size(); i++)
    {
        std::cout << "Deleting channel: " << channels[i]->name << std::endl;
        std::cout << "address: " << channels[i] << std::endl;
        
        Server::address_allocated_channels.erase(
            std::remove(Server::address_allocated_channels.begin(),
                        Server::address_allocated_channels.end(),
                        channels[i]),
            Server::address_allocated_channels.end());
        delete channels[i];
    }
    for (size_t i = 0; i < array_clients.size(); i++)
    {
        std::cout << "Deleting client: " << array_clients[i]->nickname << std::endl;
        std::cout << "address: " << array_clients[i] << std::endl;
        Server::address_allocated_clients.erase(
            std::remove(Server::address_allocated_clients.begin(),
                        Server::address_allocated_clients.end(),
                        array_clients[i]),
            Server::address_allocated_clients.end());
        delete array_clients[i];
    }
    for (size_t i = 0; i < poll_fds.size(); i++)
    {
        if (poll_fds[i].fd != -1)
            close(poll_fds[i].fd);
    }
    channels.clear();
    array_clients.clear();
    poll_fds.clear();
}

void handler(int signum)
{
    (void)signum;
    if (Server::is_server_running)
        Server::clean_server();
    show_address();
    std::exit(1);
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
    if (isnum(port) == false)
    {
        std::cerr << "error : <port> is not numeric" << std::endl;
        return 1;
    }
    long long port_num = atol(av[1]);
    if (port_num < 1024 || port_num > 49100)
    {
        std::cerr << "error : <port> is out of range" << std::endl;
        return 1;
    }

    std::string pw = av[2];
    if (pw.size() == 0)
    {
        std::cerr << "error : <password> cant be empty" << std::endl;
        return 1;
    }
    Server s(av[1], av[2]);
    s.setup();
    s.run();
    return 0;
}