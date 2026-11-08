#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Client.hpp"
#include "Command.hpp"

#define MAX_FDS 100
#define MAX_BUFF 4096

#define SERVER_NAME		"deus_irc"
#define SERVER_VERSION	"1.0"

class Channel;

class Server {
private:
	int								_servFd;
	std::string						_pswd;
	struct pollfd					_poll[MAX_FDS];
	char							_rBuff[MAX_BUFF + 1];
	char							_sBuff[MAX_FDS][MAX_BUFF * 2];

	std::map<int, Client*>			_clients;
	std::map<std::string, Channel*>	_channels;

	typedef void (Server::*CmdHandler)(Client&, const Command&);

	Server(Server const& other);
	Server& operator=(Server const& rhs);

	// Command dispatch (Registration.cpp)
	void			handleBuffer(int fd, const std::string& buf);	// one or more \r\n-terminated lines as received from fd
	void			execCommand(Client& client, const Command& cmd);

	// Clients (Registration.cpp)
	Client*			getClient(int fd);
	Client*			findClientByNick(const std::string& nick);
	void			disconnectClient(Client& client);

	// Replies (Registration.cpp)
	void			sendRaw(Client& client, const std::string& msg);
	void			sendReply(Client& client, const std::string& code, const std::string& params);

	// Registration commands (Registration.cpp)
	void			cmdPass(Client& client, const Command& cmd);
	void			cmdNick(Client& client, const Command& cmd);
	void			cmdUser(Client& client, const Command& cmd);
	void			cmdCap(Client& client, const Command& cmd);
	void			cmdQuit(Client& client, const Command& cmd);
	void			cmdMode(Client& client, const Command& cmd);
	void			modeOperator(Client& client, Channel* channel, const Command& cmd, size_t& arg, bool sign);
	void			tryRegister(Client& client);

	// JOIN (Join.cpp)
	void			cmdJoin(Client& client, const Command& cmd);
	void			joinChannel(Client& client, const std::string& name, const std::string& key);
	void			partAllChannels(Client& client);
	void			sendNamesList(Client& client, Channel& channel);

	// Broadcast (Server.cpp)
	void			broadcast(Channel& channel, const std::string& msg, Client* except = NULL);	// every member, minus except
	void			broadcastToPeers(Client& client, const std::string& msg);	// once to each client sharing a channel with client, client excluded

public:
	Server(int prtNum, std::string pswd);
	~Server();

	// Getters
	int				getSrvFd();
	struct pollfd*	getPoll();

	// Connections & I/O
	bool			setNonBlocking(int fd);
	int				findFreeInd();
	void			pollFds(int fd, int index, int events, int revents);
	bool			cnctClient();
	void			recvMsg(int i);
	void			errHandl(std::string msg);

	// Channel registry
	Channel*		findChannel(std::string const& name);
	Channel*		createChannel(std::string const& name);
	void			removeChannel(std::string const& name);
	void			removeClientFromChannels(Client* client);
};

#endif
