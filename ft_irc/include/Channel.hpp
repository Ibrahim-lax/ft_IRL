/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosabir <yosabir@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/16 06:54:56 by mjuicha           #+#    #+#             */
/*   Updated: 2025/08/19 10:07:25 by yosabir          ###   ########.fr       */
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
    protected:
        bool invite_only;
        
    public:
        std::string name;
        int admin_socket_fd;
        std::vector<Client*> clients;
        Channel() : admin_socket_fd(-1) {}
        Channel(std::string channel_name, int admin_fd) 
            : name(channel_name), admin_socket_fd(admin_fd) {}
        void setInviteOnly(bool pm);
};

#endif