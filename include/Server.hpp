#ifndef SERVER_HPP
#define SERVER_HPP
#include <list>
#include <map>
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

#include "Client.hpp"
#include "Command.hpp"

#define MAX_FDS 100
#define MAX_BUFF 4096

#define SERVER_NAME		"deus_irc"
#define SERVER_VERSION	"1.0"

class Server {
	private:
		int _servFd;
		// struct sockaddr_in _servAddr;
		std::string _pswd;
		struct pollfd _poll[MAX_FDS];
		char _rBuff[MAX_BUFF + 1];
		char _sBuff[MAX_FDS][MAX_BUFF * 2];

		// --- Registration (Registration.cpp) ---
		std::map<int, Client*> _clients;
		typedef void (Server::*CmdHandler)(Client&, const Command&);

		// void handleLine(int fd, const std::string& line);	// one line, \r\n stripped
		void handleBuffer(int fd, const std::string& buf); // one or more \r\n-terminated lines as received from fd
		void execCommand(Client& client, const Command& cmd);

		Client* getClient(int fd);
		Client* findClientByNick(const std::string& nick);
		void disconnectClient(Client& client);

		void sendRaw(Client& client, const std::string& msg);
		void sendReply(Client& client, const std::string& code, const std::string& params);

		void cmdPass(Client& client, const Command& cmd);
		void cmdNick(Client& client, const Command& cmd);
		void cmdUser(Client& client, const Command& cmd);
		void cmdCap(Client& client, const Command& cmd);
		void cmdQuit(Client& client, const Command& cmd);
		void tryRegister(Client& client);

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
