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

#define MAX_FDS 100
#define MAX_BUFF 4096

class Channel;

class Server {
private:
	int								_servFd;
	struct sockaddr_in				_servAddr;
	std::string						_pswd;
	struct pollfd					_poll[MAX_FDS];
	char							_rBuff[MAX_BUFF + 1];
	char							_sBuff[MAX_FDS][MAX_BUFF * 2];

	std::map<std::string, Channel*>	_channels;

	Server(Server const& other);
	Server& operator=(Server const& rhs);

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
};

#endif
