/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kasingh <kasingh@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 23:38:39 by pscala            #+#    #+#             */
/*   Updated: 2025/06/04 01:03:09 by kasingh          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"

void CheckSyscall(const int res, const std::string& context)
{
    if (res < 0)
    {
        std::string err = "\n" + context + ": " + std::strerror(errno);
        throw std::runtime_error(err);
    }
}

t_parsed_command parseIrcCommand(const std::string line)
{
	t_parsed_command result;
	result.prefix = "";
	result.command = "";
	result.params.clear();

	std::string remaining = line ;

	// Etape 1 on extrait le prefix;
	if(!remaining.empty() && remaining[0] == ':')
	{
		size_t space = remaining.find(' ');
		if(space != std::string::npos)
		{
			result.prefix = remaining.substr(1, space - 1);
			remaining = remaining.substr(space + 1);
		}
		else
			return result;
	}

	std::istringstream iss(remaining);
	std::string token;
	bool trailingStarted = false;

	// Etap	2 on extrait la commande
	if(!(iss >> result.command))
		return result;

	// Etape 3 on extrait les 14 patametre
	while (iss >> token)
	{
		if(token[0] == ':' && !trailingStarted)
		{
			trailingStarted = true;

			break;
		}
		else
		{
			if(result.params.size() < 14)
				result.params.push_back(token);
			else
			{
				trailingStarted = true;
				break;
			}
		}
	}
	if (trailingStarted)
	{
		std::string trailing;
		if (token[0] == ':')
			trailing = token.substr(1);
		std::string rest;
		std::getline(iss, rest);
		trailing += rest;
		result.params.push_back(trailing);
	}
	return(result);
}

std::string toLower(const std::string& str)
{
    std::string result = str;
    for (std::string::size_type i = 0; i < result.length(); ++i)
        result[i] = std::tolower(result[i]);
    return result;
}
