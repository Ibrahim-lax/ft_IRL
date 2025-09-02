/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosabir <yosabir@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/16 06:54:56 by mjuicha           #+#    #+#             */
/*   Updated: 2025/09/02 03:07:18 by yosabir          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include "Server.hpp"
#include "Client.hpp"


class Channel
{
    public:
        std::string name;
        std::string topic;
        int admin_socket_fd;
        int second_admin_socket_fd;
        std::vector<Client*> clients;
        std::string password;
        int limit;
        bool is_invite_only;
        std::vector<Client*> invited;
        bool is_password_required;
        bool is_limit_set;
        bool is_topic_restricted; // like +t mode 
        Channel() : admin_socket_fd(-1), second_admin_socket_fd(-1), is_invite_only(false), is_password_required(false), password(""), is_limit_set(false)
            {}
        Channel(std::string channel_name, int admin_fd)
            : name(channel_name), admin_socket_fd(admin_fd), second_admin_socket_fd(-1), is_invite_only(false), is_password_required(false), password(""), is_limit_set(false)
             {}
        bool operator==(const Channel &other) const {
            return name == other.name; 
        }
};


void topic(Client *client, std::string &message, Server *server);
void privmsg(Client *client, std::string &message, Server *server);
void mode(Client *client, std::string &message, Server *server);



#endif