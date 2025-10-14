/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 17:12:48 by librahim          #+#    #+#             */
/*   Updated: 2025/10/14 14:52:07 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

// int main(int ac, char *av[])
// {
//     signal(SIGPIPE, SIG_IGN);

//     Server s("6667", "0");
//     s.setup();
//     s.run();
//     return 0;
// }
// void l() {system("leaks Irc_server");}
int main(int ac, char *av[])
{
    // atexit(l);
    signal(SIGPIPE, SIG_IGN);

    if (ac < 3) {
        std::cerr << "Usage: " << av[0] << " <port> <password>\n";
        return 1;
    }

    Server s(av[1], av[2]);
    s.setup();
    s.run();
    
    return 0;
}