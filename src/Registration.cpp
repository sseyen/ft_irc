#include "Server.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include "Replies.hpp"
#include <ctime>
#include <cctype>

static const size_t NICK_MAX_LEN = 9;

//CLIENTS/////////////////////////////////////////////////////

// Returns the Client attached to fd, creating it on first contact
Client* Server::getClient(int fd)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
		return it->second;
	Client* client = new Client(fd);
	_clients[fd] = client;
	return client;
}

Client* Server::findClientByNick(const std::string& nick)
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == nick)
			return it->second;
	}
	return NULL;
}

// Closes the socket, frees its poll slot and deletes the Client.
// client is dangling afterwards: callers must not touch it again.
void Server::disconnectClient(Client& client)
{
	int fd = client.getFd();
	for (int i = 1; i < MAX_FDS; ++i)
	{
		if (_poll[i].fd == fd)
		{
			pollFds(-1, i, 0, 0);
			std::memset(_sBuff[i], 0, MAX_BUFF * 2);
			break;
		}
	}
	close(fd);
	_clients.erase(fd);
	delete &client;
}

//OUTPUT//////////////////////////////////////////////////////

void Server::sendRaw(Client& client, const std::string& msg)
{
	std::string line = msg + "\r\n";
	if (send(client.getFd(), line.c_str(), line.size(), 0) == -1)
		std::cerr << "Error using send\n";
}

// :<server> <code> <nick|*> <params>
void Server::sendReply(Client& client, const std::string& code, const std::string& params)
{
	const std::string& nick = client.getNickname();
	sendRaw(client, std::string(":") + SERVER_NAME + " " + code + " "
		+ (nick.empty() ? "*" : nick) + " " + params);
}


//COMMANDS HELPERS/////////////////////////////////////////////

static bool isSpecial(char c)
{
	return std::strchr("[]\\`_^{|}", c) != NULL;
}

// letter/special first, then letters, digits, specials, '-'
static bool isValidNick(const std::string& nick)
{
	if (nick.empty() || nick.size() > NICK_MAX_LEN)
		return false;
	if (!std::isalpha(static_cast<unsigned char>(nick[0])) && !isSpecial(nick[0]))
		return false;
	for (size_t i = 1; i < nick.size(); ++i)
	{
		unsigned char c = nick[i];
		if (!std::isalnum(c) && !isSpecial(c) && c != '-')
			return false;
	}
	return true;
}
//COMMANDS////////////////////////////////////////////////////

// QUIT [:<reason>]
void Server::cmdQuit(Client& client, const Command& cmd)
{
	std::string reason = cmd.getParamCount() ? cmd.getParam(0) : "Client Quit";
	// TODO: relay ":<prefix> QUIT :Quit: <reason>" to channel members
	sendRaw(client, "ERROR :Closing Link: (Quit: " + reason + ")");
	disconnectClient(client);
}

// PASS <password>
void Server::cmdPass(Client& client, const Command& cmd)
{
	if (client.isRegistered())
		return sendReply(client, ERR_ALREADYREGISTERED, ":You may not reregister");
	if (cmd.getParamCount() < 1)
		return sendReply(client, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
	// A wrong password is only reported in tryRegister, once NICK and USER are in
	client.setPassOk(cmd.getParam(0) == _pswd);
}

// NICK <nickname>
void Server::cmdNick(Client& client, const Command& cmd)
{
	if (cmd.getParamCount() < 1 || cmd.getParam(0).empty())
		return sendReply(client, ERR_NONICKNAMEGIVEN, ":No nickname given");

	const std::string& nick = cmd.getParam(0);
	if (!isValidNick(nick))
		return sendReply(client, ERR_ERRONEUSNICKNAME, nick + " :Erroneus nickname");

	Client* owner = findClientByNick(nick);
	if (owner == &client)
		return;
	if (owner)
		return sendReply(client, ERR_NICKNAMEINUSE, nick + " :Nickname is already in use");

	if (client.isRegistered())
	{
		// TODO: also relay to every client sharing a channel
		std::string oldPrefix = client.getPrefix();
		client.setNickname(nick);
		return sendRaw(client, ":" + oldPrefix + " NICK :" + nick);
	}
	client.setNickname(nick);
	tryRegister(client);
}

// USER <username> <mode> <unused> :<realname>
// "USER guest 0 * :Ronnie Reagan"
void Server::cmdUser(Client& client, const Command& cmd)
{
	if (client.isRegistered())
		return sendReply(client, ERR_ALREADYREGISTERED, ":You may not reregister");
	if (cmd.getParamCount() < 4 || cmd.getParam(0).empty())
		return sendReply(client, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");

	client.setUsername(cmd.getParam(0));
	client.setRealname(cmd.getParam(3));
	tryRegister(client);
}

void Server::cmdCap(Client& client, const Command& cmd)
{
	if (cmd.getParam(0) == "LS")
		sendRaw(client, std::string(":") + SERVER_NAME + " CAP * LS :");
}
static bool allowedBeforeRegistration(const std::string& name)
{
	return name == "PASS" || name == "NICK" || name == "USER"
		|| name == "CAP" || name == "QUIT";
}

void Server::execCommand(Client& client, const Command& cmd)
{
	static std::map<std::string, CmdHandler> handlers;
	if (handlers.empty())
	{
		handlers["PASS"] = &Server::cmdPass;
		handlers["NICK"] = &Server::cmdNick;
		handlers["USER"] = &Server::cmdUser;
		handlers["CAP"]  = &Server::cmdCap;
		handlers["QUIT"] = &Server::cmdQuit;
		handlers["MODE"] = &Server::cmdMode;
	}

	const std::string& name = cmd.getCommand();
	if (!client.isRegistered() && !allowedBeforeRegistration(name))
		return sendReply(client, ERR_NOTREGISTERED, ":You have not registered");

	std::map<std::string, CmdHandler>::iterator it = handlers.find(name);
	if (it == handlers.end())
		return sendReply(client, ERR_UNKNOWNCOMMAND, cmd.getRawCommand() + " :Unknown command");
	// handler may disconnect (and delete) client: nothing may follow this call
	(this->*(it->second))(client, cmd);
}

// Completes registration once NICK and USER are both known
void Server::tryRegister(Client& client)
{
	if (client.isRegistered() || client.getNickname().empty() || client.getUsername().empty())
		return;

	if (!client.isPassOk())
	{
		sendReply(client, ERR_PASSWDMISMATCH, ":Password incorrect");
		sendRaw(client, "ERROR :Closing Link: (Password incorrect)");
		return disconnectClient(client);
	}

	client.setRegistered();

	sendReply(client, RPL_WELCOME, std::string(":Welcome to the ") + SERVER_NAME
		+ " Network, " + client.getPrefix());
	sendReply(client, RPL_YOURHOST, std::string(":Your host is ") + SERVER_NAME
		+ ", running version " + SERVER_VERSION);
	sendReply(client, RPL_CREATED, std::string(":This server was created today"));
	sendReply(client, RPL_MYINFO, std::string(SERVER_NAME) + " " + SERVER_VERSION + " io itkol kol");
}


// void Server::handleLine(int fd, const std::string& line)
// {
// 	Command cmd;
//
// 	if (!cmd.parse(line))
// 		return ;
// 	execCommand(*getClient(fd), cmd);
// }


// buf: recvMsg's buffer, one or more \r\n-terminated lines
void Server::handleBuffer(int fd, const std::string& buf)
{
	size_t start = 0;
	size_t end;

	while ((end = buf.find('\n', start)) != std::string::npos)
	{
		std::string line = buf.substr(start, end - start);
		start = end + 1;
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		Command cmd;
		if (!cmd.parse(line))
			continue;
		execCommand(*getClient(fd), cmd);
		if (_clients.find(fd) == _clients.end())	// QUIT or failed registration closed it
			return;
	}
}
