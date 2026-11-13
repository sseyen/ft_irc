#include "Server.hpp"
#include "Channel.hpp"
#include "Command.hpp"
#include "Client.hpp"
#include <set>

/*** @brief Server constructor with port number and password args
 * Creates a server socket fd with IPv4 family and TCP connection.
 * Sets server to non-blocking. It doesn't wait indefinitely for action.
 * Stores all variables that describes network adress.
 * Converts addresses from host byte order into network byte order.
 * Binds associated address name with corresponding address and port.
 * Starts to listen to specified number of incoming TCP connections.
 * Registers server socket and initializes connection slots to listen events.
 * In general it registers server address, starts to listen connection and events.
*/
Server::Server(int prtNum, std::string pswd)
{
	_pswd = pswd;
	_servFd = socket(AF_INET, SOCK_STREAM, 0);
	if(_servFd == -1)
		errHandl("Error oppening socket\n");
	int opt = 1;
	if (setsockopt(_servFd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt)) < 0)
	{
		close(_servFd);
		errHandl("Error using setsockopt\n");
	}
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
	for(int i = 0; i < MAX_FDS; i++)
		std::memset(_sBuff[i], 0, MAX_BUFF * 2);
	// Here POLLIN means that incoming connection is ready to be accepted
	pollFds(_servFd, 0, POLLIN, 0);
	for(int i = 1; i < MAX_FDS; i++)
		pollFds(-1, i, 0, 0);
}

Server::~Server()
{
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		delete it->second;

	for (int i = 0; i < MAX_FDS; ++i)
		if (_poll[i].fd != -1)
			close(_poll[i].fd);

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

/*** @brief Changes properties of descriptor with file flag and sets it to non-blocking */
bool Server::setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return false;
	return true;
}

/*** @brief Finds free index in _poll registry */
int Server::findFreeInd()
{
	for(int i = 1; i < MAX_FDS; i++)
	{
		if(_poll[i].fd == -1)
			return i;
	}
	return -1;
}

/*** @brief Registers server socket and initializes connection slots to listen events. */
void Server::pollFds(int fd, int index, int events, int revents)
{
	_poll[index].fd = fd;
	_poll[index].events = events;
	_poll[index].revents = revents;
}

/*** @brief Here listening socket accepts newly dedicated socket. 
 * Makes it non-blocking. It returns -1 when there nothing to receive.
 * Finds and registers client socket on a free space.
*/
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

/*** @brief Receives data from the client socket into the buffer of MAX_BUFF. 
 * Makes it non-blocking. It returns -1 when there nothing to receive.
 * If 0 or minus value received disconnect user from server.
 * Writes out a message terminated with \r\n or adds to save buffer.
*/
void Server::recvMsg(int i)
{
	std::memset(_rBuff, 0, MAX_BUFF);
	int chk_r = recv(_poll[i].fd, _rBuff, MAX_BUFF, 0);
	int fd = _poll[i].fd;
	if(chk_r > 0)
		_rBuff[chk_r] = '\0';
	size_t sbuff_l = strlen(_sBuff[i]);
	if(chk_r <= 0 || sbuff_l + chk_r >= sizeof(_sBuff[i]))
	{
		if(sbuff_l + chk_r >= sizeof(_sBuff[i]))
			std::cerr << "Socket " << fd << ": message too long\n";
		else if(chk_r < 0)
			std::cerr << "Error receiving data\n";
		std::cout << "Socket " << fd << ": disconnected from the server\n";
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
		std::cout << "Socket " << _poll[i].fd << ": ";
		std::cout << _sBuff[i] << '\n';
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

// case insensitive lowercasing for channel/nick names lookups
std::string ircLower(const std::string& s)
{
	std::string folded = s;

	for (size_t i = 0; i < folded.size(); ++i)
	{
		if (folded[i] >= 'A' && folded[i] <= '^')
			folded[i] += 'a' - 'A';
	}
	return folded;
}

Channel* Server::findChannel(std::string const& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(ircLower(name));
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
		_channels[ircLower(name)] = channel;
	}
	return channel;
}

void Server::removeChannel(std::string const& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(ircLower(name));
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

