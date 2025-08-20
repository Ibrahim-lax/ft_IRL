/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/16 06:54:56 by mjuicha           #+#    #+#             */
/*   Updated: 2025/08/20 15:19:56 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include "Server.hpp"
#include "Client.hpp"

class Server;
class Client;

class Channel
{
    public:
        std::string name;
        int admin_socket_fd;
        std::vector<Client*> clients;
        bool is_invite_only;
        Channel() : admin_socket_fd(-1), is_invite_only(false) 
            {}
        Channel(std::string channel_name, int admin_fd) 
            : name(channel_name), admin_socket_fd(admin_fd), is_invite_only(false)
             {}
};

#endif