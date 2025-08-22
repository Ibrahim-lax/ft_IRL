/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/13 16:12:52 by librahim          #+#    #+#             */
/*   Updated: 2025/08/22 16:27:09 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP
#include "Server.hpp"

class Server;
class Channel;

class Client
{
    public :
        bool        is_auth;
        bool        is_registered;
        int         socket_fd;
        bool        is_nickname;
        bool        is_username;
        std::string nickname;
        std::string username;
        std::string real_name;
        std::vector<Channel*> channelsjoined;
        std::vector<Channel*> banned_channels;
        std::vector<Channel*> invited_channels;
        
        Client() {
            // std::cout << "Client created" << std::endl;
            is_auth = false;
            is_registered = false;
            is_nickname = false;
            is_username = false;
        }
        Client(int fd) : socket_fd(fd), is_auth(false), is_registered(false)
        , is_nickname(false), is_username(false)
        {
            // std::cout << "Client created with fd: " << fd << std::endl;
        }
};


#endif