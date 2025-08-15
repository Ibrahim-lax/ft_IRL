/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/13 16:12:52 by librahim          #+#    #+#             */
/*   Updated: 2025/08/15 02:44:37 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP
#include "Server.hpp"
#include "Registration.hpp"

class Server;

class Client
{
    public :
        int id;
        std::string nickname;
        std::string username;
        std::string real_name;
        Registration reg;
        Client() {
            std::cout << "Client created" << std::endl;
            reg.is_auth = false;
        }
};


#endif