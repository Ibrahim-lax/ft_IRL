/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Registration.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/14 02:59:27 by mjuicha           #+#    #+#             */
/*   Updated: 2025/08/15 02:44:31 by mjuicha          ###   ########.fr       */
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
        std::string message;
        std::string password;
        int socket_fd;
        Registration() : is_auth(false) {};

};

#endif