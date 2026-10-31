#include "../include/Server.hpp"

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
    	errHandl("Error while binding\n");;
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
	if(chk_r < 0)
	{
		std::cerr << "Error using recv\n";
		close(fd);
		pollFds(-1, i, 0, 0);
	}
	if(chk_r == 0)
	{
		std::strcat(_sBuff[i], "QUIT\r\n\n");
		std::cout << "Client " << _poll[i].fd << " disconnected from the server\n";
		std::cout << _sBuff[i];
		//execute QUIT
		close(fd);
		pollFds(-1, i, 0, 0);
		std::memset(_sBuff[i], 0, MAX_BUFF * 2);
	}
	if(chk_r >= 2 && _rBuff[chk_r - 1] == '\n' && _rBuff[chk_r - 2] == '\r')
	{
		std::strcat(_sBuff[i], _rBuff);
		std::cout << "Client " << _poll[i].fd << " wrote: ";
		std::cout << _sBuff[i] << '\n';
		//execute cmnd
		std::memset(_sBuff[i], 0, MAX_BUFF * 2);
	}
	else
		std::strcat(_sBuff[i], _rBuff);
}

void Server::pollFds(int fd, int index, int events, int revents)
{
	_poll[index].fd = fd;
	_poll[index].events = events;
	_poll[index].revents = revents;
}

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

void Server::errHandl(std::string msg) {
	std::cerr << msg;
	_exit(1);
}

struct pollfd* Server::getPoll() {
	return _poll;
}

int Server::getSrvFd() {
	return _servFd;
}