#ifndef SERVER_HPP
#define SERVER_HPP
#include <list>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <cstdlib>
#include <map>
#include <string>

#define MAX_FDS 100
#define MAX_BUFF 4096

class Channel;

class Server {
	private:
		std::map<std::string, Channel*> _channels;
		int _servFd;
		struct sockaddr_in _servAddr;
		std::string _pswd;
		struct pollfd _poll[MAX_FDS];
		char _rBuff[MAX_BUFF + 1];
		char _sBuff[MAX_FDS][MAX_BUFF * 2];
		Server& operator=(Server const& rhs);
		Server(Server const& other);

	public:
		Server(int prtNum, std::string pswd);
		~Server();
		Channel* findChannel(std::string const& name);
		Channel* createChannel(std::string const& name);
		void removeChannel(std::string const& name);
		int findFreeInd();
		bool cnctClient();
		void recvMsg(int i);
		bool setNonBlocking(int fd);
		void errHandl(std::string msg);
		int getSrvFd();
		struct pollfd* getPoll();
		void pollFds(int fd, int index, int events, int revents);
};
#endif