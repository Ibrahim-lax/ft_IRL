/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuicha <mjuicha@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 17:12:48 by librahim          #+#    #+#             */
/*   Updated: 2025/09/01 13:42:25 by mjuicha          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"



int main(int ac, char *av[])
{
    signal(SIGPIPE, SIG_IGN);
    (void)ac;
    (void)av;
    Server s("6667", "123");
    s.setup();
    s.run();
    return 0;
}
