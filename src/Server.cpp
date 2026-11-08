#include "Server.hpp"
#include "Channel.hpp"
#include "Command.hpp"
#include "Client.hpp"
#include <set>

Server::Server(int prtNum, std::string pswd)
{
	_pswd = pswd;
	_servFd = socket(AF_INET, SOCK_STREAM, 0);
	if(_servFd == -1)
		errHandl("Error oppening socket\n");
	if (!setNonBlocking(_servFd))
	{
		close(_servFd);
		errHandl("Error using fcntl\n");
	}
	sockaddr_in server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(prtNum);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_servFd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		close(_servFd);
		errHandl("Error while binding\n");
	}
	if (listen(_servFd, SOMAXCONN) == -1)
	{
		close(_servFd);
		errHandl("Error while listening\n");
	}
	pollFds(_servFd, 0, POLLIN, 0);
	for(int i = 1; i < MAX_FDS; i++)
		pollFds(-1, i, 0, 0);
}

Server::~Server()
{
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}

	close(_servFd);
}

/* ************************************************************************** */
/*                                  Getters                                   */
/* ************************************************************************** */

int Server::getSrvFd()
{
	return _servFd;
}

struct pollfd* Server::getPoll()
{
	return _poll;
}

/* ************************************************************************** */
/*                              Connections & I/O                             */
/* ************************************************************************** */

bool Server::setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return false;
	return true;
}

int Server::findFreeInd()
{
	for(int i = 1; i < MAX_FDS; i++)
	{
		if(_poll[i].fd == -1)
			return i;
	}
	return -1;
}

void Server::pollFds(int fd, int index, int events, int revents)
{
	_poll[index].fd = fd;
	_poll[index].events = events;
	_poll[index].revents = revents;
}

bool Server::cnctClient()
{
	sockaddr_in cli_addr = {};
	socklen_t cli_len = sizeof(cli_addr);
	int cli_fd = accept(_servFd, (sockaddr *)&cli_addr, &cli_len);
	if(cli_fd == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return false;
		std::cerr << "Error: " << std::strerror(errno) << '\n';
		return false;
	}
	if (!setNonBlocking(cli_fd))
	{
		close(cli_fd);
		std::cerr << "Error using fcntl\n";
		return false;
	}
	int i = findFreeInd();
	if(i == -1)
	{
		close(cli_fd);
		std::cerr << "Error of too much client entries\n";
		return false;
	}
	pollFds(cli_fd, i, POLLIN | POLLHUP, 0);
	return true;
}

void Server::recvMsg(int i)
{
	std::memset(_rBuff, 0, MAX_BUFF);
	int chk_r = recv(_poll[i].fd, _rBuff, MAX_BUFF, MSG_DONTWAIT);
	int fd = _poll[i].fd;
	if (chk_r > 0)
		_rBuff[chk_r] = '\0';
	if(chk_r <= 0)
	{
		std::cout << "Client " << fd << " disconnected from the server\n";
		std::map<int, Client*>::iterator it = _clients.find(fd);
		if (it != _clients.end())
		{
			broadcastToPeers(*it->second, ":" + it->second->getPrefix() + " QUIT :Connection closed");
			disconnectClient(*it->second);
		}
		else
		{
			close(fd);
			pollFds(-1, i, 0, 0);
		}
		return;
	}
	if(chk_r >= 2 && _rBuff[chk_r - 1] == '\n' && _rBuff[chk_r - 2] == '\r')
	{
		std::strcat(_sBuff[i], _rBuff);
		std::cout << "Client " << _poll[i].fd << " wrote: ";
		std::cout << _sBuff[i] << '\n';
		//execute cmnd
		handleBuffer(fd, std::string(_sBuff[i]));
		std::memset(_sBuff[i], 0, MAX_BUFF * 2);
	}
	else
		std::strcat(_sBuff[i], _rBuff);
}

void Server::errHandl(std::string msg)
{
	std::cerr << msg;
	_exit(1);
}

/* ************************************************************************** */
/*                              Channel registry                              */
/* ************************************************************************** */

Channel* Server::findChannel(std::string const& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it == _channels.end())
		return NULL;
	return it->second;
}

Channel* Server::createChannel(std::string const& name)
{
	Channel* channel = findChannel(name);
	if (!channel)
	{
		channel = new Channel(name);
		_channels[name] = channel;
	}
	return channel;
}

void Server::removeChannel(std::string const& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it == _channels.end())
		return;
	delete it->second;
	_channels.erase(it);
}

void Server::removeClientFromChannels(Client* client) {
	std::vector<std::string> empty;

	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
		it->second->removeClient(client);
		it->second->removeInvite(client);
		if (it->second->getClients().empty())
			empty.push_back(it->first);
	}

	for (size_t i = 0; i < empty.size(); i++)
		removeChannel(empty[i]);
}

/* ************************************************************************** */
/*                                 Broadcast                                  */
/* ************************************************************************** */

// msg broadcast to members of channel, except except
void Server::broadcast(Channel& channel, const std::string& msg, Client* except)
{
	const std::vector<Client*>& members = channel.getClients();

	for (size_t i = 0; i < members.size(); i++)
	{
		if (members[i] != except)
			sendRaw(*members[i], msg);
	}
}

// msg broadcast to each client sharing a channel with client, client excluded
void Server::broadcastToPeers(Client& client, const std::string& msg)
{
	const std::vector<Channel*>& channels = client.getChannels();
	std::set<Client*> peers;

	for (size_t i = 0; i < channels.size(); i++)
	{
		const std::vector<Client*>& members = channels[i]->getClients();
		peers.insert(members.begin(), members.end());
	}
	peers.erase(&client);
	for (std::set<Client*>::iterator it = peers.begin(); it != peers.end(); ++it)
		sendRaw(**it, msg);
}

