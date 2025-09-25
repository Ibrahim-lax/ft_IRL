/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosabir <yosabir@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/16 06:54:56 by mjuicha           #+#    #+#             */
/*   Updated: 2025/09/23 16:43:50 by yosabir          ###   ########.fr       */
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
    int admin_socket_fd;               // creator
    std::vector<int> operators;        // all operators including creator
    std::vector<Client*> clients;
    std::string password;
    int limit;
    bool is_invite_only;
    std::vector<Client*> invited;
    bool is_password_required;
    bool is_limit_set;
    bool is_topic_restricted; // +t mode

    Channel() : admin_socket_fd(-1), password(""), is_invite_only(false), is_password_required(false), is_limit_set(false), is_topic_restricted(false) {}

    Channel(std::string channel_name, int admin_fd)
        : name(channel_name), admin_socket_fd(admin_fd),
          password(""), is_invite_only(false), is_password_required(false),  is_limit_set(false), is_topic_restricted(false)
    {
        operators.push_back(admin_fd); // creator is automatically an operator
    }

    bool operator==(const Channel &other) const {
        return name == other.name;
    }

    bool isOperator(int socket_fd)
    {
        if (socket_fd == admin_socket_fd)
            return true;
        for (size_t i = 0; i < operators.size(); i++)
        {
            if (operators[i] == socket_fd)
                return true;
        }
        return false;
    }

    void addOperator(int socket_fd)
    {
        if (!isOperator(socket_fd))
            operators.push_back(socket_fd);
    }

    void removeOperator(int socket_fd)
    {
        if (socket_fd == admin_socket_fd) return;
        for (size_t i = 0; i < operators.size(); i++)
        {
            if (operators[i] == socket_fd)
            {
                operators.erase(operators.begin() + i);
                break;
            }
        }
    }

    int next_operator(int socket_fd) // med version
    {
        for (size_t i = 0; i < operators.size(); i++)
        {
            if (operators[i] == socket_fd)
            {
                if (i + 1 < operators.size())
                    return operators[i + 1];
                else if (i - 1 >= 0)
                    return operators[i - 1];
            }
        }
        return -1;
    }

     void removeOperatorVV(int socket_fd) // med version
    {
        if (socket_fd == admin_socket_fd)
            admin_socket_fd = next_operator(admin_socket_fd);
        for (size_t i = 0; i < operators.size(); i++)
        {
            if (operators[i] == socket_fd)
            {
                operators.erase(operators.begin() + i);
                break;
            }
        }
    }
};


void topic(Client *client, std::string &message, Server *server);
void privmsg(Client *client, std::string &message, Server *server);
void mode(Client *client, std::string &message, Server *server);



#endif