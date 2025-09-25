/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: librahim <librahim@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/25 16:59:00 by librahim          #+#    #+#             */
/*   Updated: 2025/09/25 18:30:56 by librahim         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
# define BOT_HPP
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <iostream>

class Bot
{
    private :
        int fd;
        std::string port;        
        std::string pw;
        long bot_start_time;
    public :
        Bot(std::string port, std::string password);
        int     check();
        int     setup();
        void    run();
};

#endif