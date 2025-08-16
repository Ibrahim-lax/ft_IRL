/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/16 06:54:56 by mjuicha           #+#    #+#             */
/*   Updated: 2025/08/16 07:07:57 by mjuicha          ###   ########.fr       */
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
        std::vector<Client> clients;
};

#endif