/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Registration.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/14 02:59:27 by mjuicha           #+#    #+#             */
/*   Updated: 2025/08/14 05:12:18 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REGISTRATION_HPP
#define REGISTRATION_HPP
#include "Server.hpp"

#include <string>
#include <vector>

class Registration
{
    public:
        bool is_auth;
        std::string command;
        std::string password;
        int client_index;
        int socket_fd;
        std::vector<struct pollfd> poll_fds;
        Registration() : is_auth(false) {};

};

#endif