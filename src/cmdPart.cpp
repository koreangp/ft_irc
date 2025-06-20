/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmdPart.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pscala <pscala@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/28 01:52:05 by pscala            #+#    #+#             */
/*   Updated: 2025/06/02 03:40:47 by pscala           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Serveur.hpp"
#include "Client.hpp"
#include "Channel.hpp"

std::vector<std::string> splitCommaList(const std::string& input)
{
	std::vector<std::string> result;
	std::stringstream ss(input);
	std::string item;

	while (std::getline(ss, item, ','))
		result.push_back(item);

	return result;
}

void Serveur::cmdPart(Client &client, const std::vector<std::string>& params)
{
	if (!client.isRegistered())
	{
		sendError(client, 451, " PART ", "You have not registered");
		return;
	}

	if(params.empty())
	{
		sendError(client, 461, "PART", "Not enough parameters");
		return;
	}
	std::vector<std::string> channelNames = splitCommaList(params[0]);
	std::string partMessage = client.getNickname();
	if (params.size() > 1)
		partMessage = params[1];
	for (size_t i = 0; i <  channelNames.size(); i++)
	{
		const std::string& chanName = channelNames[i];
		Channel* chan = getChannel(chanName);
		if (!chan)
		{
			sendError(client, 403, chanName, "No such channel");
			continue;
		}
		if (!chan->isMember(&client))
		{
			sendError(client, 442, chanName, "You're not on that channel");
			continue;
		}
		std::ostringstream oss;
		oss << ":" << client.getPrefix() << " PART " << chanName;
		if (!partMessage.empty())
			oss << " :" << partMessage;
		oss << "\r\n";
		broadcastToChannel(chan, oss.str());

		client.leaveChannel(chan);
		chan->RemoveClient(&client);

		if (chan->getClients().empty())
			deleteChannel(chanName);
	}


}
