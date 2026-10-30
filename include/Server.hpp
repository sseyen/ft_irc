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

#define MAX_FDS 100
#define MAX_BUFF 4096

class Server {
	private:
		int _servFd;
		struct sockaddr_in _servAddr;
		std::string _pswd;
		struct pollfd _poll[MAX_FDS];
		char _rBuff[MAX_BUFF + 1];
		char _sBuff[MAX_FDS][MAX_BUFF * 2];
	public:
		Server(int prtNum, std::string pswd);
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