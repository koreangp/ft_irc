/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmdPass.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kasingh <kasingh@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/29 02:58:15 by kasingh           #+#    #+#             */
/*   Updated: 2025/06/02 22:40:15 by kasingh          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Serveur.hpp"

void Serveur::cmdPass(Client &client, const std::vector<std::string> &params)
{

	if (params.empty())
	{
		sendError(client, 461, "PASS", "Not enough parameters");
		return ;
	}

	if (client.isRegistered())
	{
		sendError(client, 462, "PASS", "You may not reregister");
		return ;
	}

	if(params[0] == _passwordBOT)
		client.setIsBoot(true);
	else if (params[0] != _password)
	{
		return ;
	}
	client.setHasPass(true);
}
