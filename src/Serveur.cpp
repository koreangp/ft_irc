/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Serveur.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kasingh <kasingh@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/25 15:54:45 by kasingh           #+#    #+#             */
/*   Updated: 2025/06/05 16:52:36 by kasingh          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Serveur.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Serveur::Serveur(int port, std::string &password)
{
	_port = port;
	_password = password;
	_passwordBOT = "IAMTHEBOT";
	_server_fd = -1;
	_servername = "SuperServer";
	_serverVersion = "1.0";
	_commands["NICK"] = &Serveur::cmdNick;
	_commands["USER"] = &Serveur::cmdUser;
	_commands["JOIN"] = &Serveur::cmdJoin;
	_commands["PART"] = &Serveur::cmdPart;
	_commands["PASS"] = &Serveur::cmdPass;
	_commands["PRIVMSG"] = &Serveur::cmdPrivmsg;
	_commands["NOTICE"] = &Serveur::cmdNotice;
	_commands["QUIT"] = &Serveur::cmdQuit;
	_commands["PING"] = &Serveur::cmdPing;
	_commands["KICK"] = &Serveur::cmdKick;
	_commands["INVITE"] = &Serveur::cmdInvite;
	_commands["TOPIC"] = &Serveur::cmdTopic;
	_commands["MODE"] = &Serveur::cmdMode;
}

Serveur::~Serveur()
{
	if (_server_fd > 0)
		close(_server_fd);
	for (std::vector<Client *>::const_iterator it = _clients_vec.begin(); it != _clients_vec.end(); ++it)
	{
		(*it)->close_fd();
		delete *it;
	}
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	_channels.clear();
	if (_epollfd)
		close(_epollfd);
}

void Serveur::start()
{
	struct sockaddr_in	my_sockaddr;
	struct epoll_event	ev;

	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	CheckSyscall(_server_fd, "socket()");
	setNonBlockSocket(_server_fd);
	std::memset(&my_sockaddr, 0, sizeof(my_sockaddr));
	my_sockaddr.sin_family = AF_INET;
	my_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_sockaddr.sin_port = htons(_port);
	CheckSyscall(bind(_server_fd, (struct sockaddr *)(&my_sockaddr), sizeof(my_sockaddr)), "bind()");
	CheckSyscall(listen(_server_fd, 1000), "listen()");
	ev.events = EPOLLIN;
	ev.data.fd = _server_fd;
	_epollfd = epoll_create1(0);
	CheckSyscall(_epollfd, "epoll_create1()");
	CheckSyscall(epoll_ctl(_epollfd, EPOLL_CTL_ADD, _server_fd, &ev), "epoll_ctl()");
}

void Serveur::run()
{
	int client_fd;
	int nfds = epoll_wait(_epollfd, _events, MAXEVENTS, -1);
	if (nfds == -1)
	{
		if (errno != EINTR)
			CheckSyscall(nfds, "epoll_wait()");
		return;
	}
	for (int i = 0; i < nfds; ++i)
	{
		if (_events[i].data.fd == _server_fd)
		{
			struct epoll_event ev;
			client_fd = accept(_server_fd, 0, 0);
			CheckSyscall(client_fd, "accept()");
			setNonBlockSocket(client_fd);
			ev.data.fd = client_fd;
			ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
			CheckSyscall(epoll_ctl(_epollfd, EPOLL_CTL_ADD, client_fd, &ev), "epoll_ctl_ADD()");
			Client* newClient = new Client(client_fd);
			_clients_vec.push_back(newClient);
			std::cout << BGREEN << "Accepted client with fd: " << BWHITE << client_fd << RESET << std::endl;
		}
		else
		{
			handleClientEvents(_events[i]);
		}
	}
}

void Serveur::handleClientEvents(const struct epoll_event& ev)
{
	Client *client = FindClient(ev.data.fd);
	if (!client)
		return;

	if (ev.events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
	{
		removeClient(client);
		return;
	}

	if (ev.events & EPOLLIN)
	{
		char buffer[1024];
		int n = recv(client->getFd(), buffer, sizeof(buffer), 0);
		if (n <= 0)
		{
			removeClient(client);
			return;
		}

		try {
			client->FillReadBuffer(std::string(buffer, n));
		}
		catch (...) {
			removeClient(client);
			return;
		}

		std::string line;
		while (client->getCmdNextLine(line))
		{
			handleClientCommand(*client, line);
			if (!FindClient(ev.data.fd))
				return;
		}
	}

	if (ev.events & EPOLLOUT)
	{
		int n = TryToSend(*client, client->getWriteBuffer());
		if (n == static_cast<int>(client->getWriteBuffer().length()))
		{
			client->FillWriteBuffer("");
			disableWriteEvent(*client);
		}
	}
}


Client *Serveur::FindClient(const int fd)
{
	for (std::vector<Client *>::iterator it = _clients_vec.begin(); it != _clients_vec.end(); ++it)
		if ((*it)->getFd() == fd)
			return (*it);
	return NULL;
}




void	Serveur::handleClientCommand(Client &client, std::string line)
{
	t_parsed_command cmd = parseIrcCommand(line);

	std::cout << "================= Parsed Command =================" << std::endl;
	std::cout << "Raw line: \"" << line << "\"" << std::endl;

	if (!cmd.prefix.empty())
		std::cout << "Prefix   : [" << cmd.prefix << "]" << std::endl;
	else
		std::cout << "Prefix   : [none]" << std::endl;

	std::cout << "Command  : [" << cmd.command << "]" << std::endl;

	std::cout << "Params   : (" << cmd.params.size() << " param(s))" << std::endl;
	for (size_t i = 0; i < cmd.params.size(); ++i)
		std::cout << "  Param[" << i + 1 << "] : [" << cmd.params[i] << "]" << std::endl;

	std::cout << "==================================================" << std::endl;

	std::map<std::string, CommandFunc>::iterator it = _commands.find(cmd.command);
	if (it != _commands.end())
	{
		CommandFunc func = it->second;
		(this->*func)(client, cmd.params);
	}
	else if (cmd.command != "CAP")
		sendError(client, 421, cmd.command, "Unknown command");

}

void Serveur::setNonBlockSocket(const int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	CheckSyscall(flags, "get fcntl()");
	CheckSyscall(fcntl(fd, F_SETFL, flags | O_NONBLOCK), "set fcntl()");
}


int Serveur::TryToSend(Client &client, std::string msg)
{
	ssize_t sent = send(client.getFd(), msg.c_str(), msg.length(), MSG_NOSIGNAL);
	if (sent < 0)
	{
		if (errno == EWOULDBLOCK)
		{
			client.FillWriteBuffer(msg);
			enableWriteEvent(client);
		}
		else
			removeClient(&client);
	}
	else if (sent < (ssize_t)msg.length())
	{
		client.FillWriteBuffer(msg.substr(sent));
		enableWriteEvent(client);
	}
	return(sent);
}

void Serveur::sendError(Client& client, int code, const std::string& arg, const std::string& message)
{
	std::ostringstream oss;
	oss << ":" << _servername << " "<< code << " "
	    << (client.getNickname().empty() ? "*" : client.getNickname()) << " "
	    << arg << " :" << message << "\r\n";
	TryToSend(client, oss.str());
}

void Serveur::enableWriteEvent(Client& client)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.data.fd = client.getFd();
	ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR;

	CheckSyscall(epoll_ctl(_epollfd, EPOLL_CTL_MOD, client.getFd(), &ev),  "epoll_ctl_ADD()");

}
void Serveur::disableWriteEvent(Client& client)
{
	struct epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.data.fd = client.getFd();
	ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;

	CheckSyscall(epoll_ctl(_epollfd, EPOLL_CTL_MOD, client.getFd(), &ev),  "epoll_ctl_ADD()");
}

void Serveur::sendWelcomeMessages(Client &client)
{
	std::ostringstream msg;

	msg << ":" << _servername << " 001 " << client.getNickname()
	    << " :Welcome to the Internet Relay Network "
	    << client.getNickname() << "!" << client.getUsername()
	    << "@localhost\r\n";
	TryToSend(client, msg.str());
	msg.str("");
	msg.clear();

	msg << ":" << _servername << " 002 " << client.getNickname()
	    << " :Your host is " << _servername
	    << ", running version " << _serverVersion << "\r\n";
	TryToSend(client, msg.str());
	msg.str("");
	msg.clear();

	msg << ":" << _servername << " 003 " << client.getNickname()
	    << " :This server was created " << __DATE__ << "\r\n";
	TryToSend(client, msg.str());
	msg.str("");
	msg.clear();

	msg << ":" << _servername << " 004 " << client.getNickname()
	    << " " << _servername << " " << _serverVersion
	    << " o oitlks\r\n";
	TryToSend(client, msg.str());
}

Channel* Serveur::getChannel(const std::string& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

Channel* Serveur::getOrCreateChannel(const std::string& name)
{
	Channel* chan = getChannel(name);
	if (!chan)
	{
		chan = new Channel(name);
		_channels[name] = chan;
	}
	return chan;
}

void Serveur::broadcastToChannel(Channel* channel, const std::string& message)
{
	const std::set<Client*>& members = channel->getClients();
	std::cout << "Client dans le channel: " << channel->getChannelName() << " : " <<  members.size() << std::endl;
	for (std::set<Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
		TryToSend(**it, message);
}

void	Serveur::removeClient(Client *client)
{
	CheckSyscall(epoll_ctl(_epollfd, EPOLL_CTL_DEL, client->getFd(), NULL), "epoll_ctl()");
	client->close_fd();
	std::cout << BYELLOW << "Client " << (client->getNickname().empty() ? "*" : client->getNickname()) << " disconnected." << RESET << std::endl;

	disconnectClient(client);
}

void Serveur::disconnectClient(Client* client)
{
	std::set<Channel*> chans = client->getJoinedChannels();

	for (std::set<Channel*>::iterator it = chans.begin(); it != chans.end(); ++it)
	{
	    Channel* chan = *it;

	    chan->RemoveClient(client);
	    client->leaveChannel(chan);

	    if (chan->getClients().empty())
	        deleteChannel(chan->getChannelName());
	}
	for (std::vector<Client*>::iterator it = _clients_vec.begin(); it != _clients_vec.end(); ++it)
	{
		if (*it == client)
		{
			delete *it;
			_clients_vec.erase(it);
			break;
		}
	}
}

void Serveur::deleteChannel(const std::string& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it != _channels.end())
	{
		delete it->second;
		_channels.erase(it);
	}
}
