#include "Server.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include "Replies.hpp"
#include <ctime>
#include <cctype>

static const size_t NICK_MAX_LEN = 9;
static const size_t USER_MAX_LEN = 10;

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
	const std::string folded = ircLower(nick);

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (ircLower(it->second->getNickname()) == folded)
			return it->second;
	}
	return NULL;
}

void Server::disconnectClient(Client& client, bool flush)
{
	removeClientFromChannels(&client);
	int fd = client.getFd();
	int i = findPollIndex(fd);
	if (flush && !client.getOutBuf().empty() && i != -1)
	{
		client.setClosing();
		_poll[i].events |= POLLOUT;
		return;
	}
	if (i != -1)
	{
		pollFds(-1, i, 0, 0);
		std::memset(_sBuff[i], 0, MAX_BUFF * 2);
	}
	close(fd);
	_clients.erase(fd);
	delete &client;
}

//OUTPUT//////////////////////////////////////////////////////

void Server::queueReply(Client& client, const std::string& msg)
{
	client.getOutBuf() += msg.substr(0, Command::MAX_LINE_SIZE) + "\r\n";
	int i = findPollIndex(client.getFd());
	if (i != -1)
		_poll[i].events |= POLLOUT;
}

// :<server> <code> <nick|*> <params>
void Server::sendReply(Client& client, const std::string& code, const std::string& params)
{
	const std::string& nick = client.getNickname();
	queueReply(client, std::string(":") + SERVER_NAME + " " + code + " "
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

void Server::cmdPing(Client& client, const Command& cmd)
{
	if (cmd.getParamCount() < 1 || cmd.getParam(0).empty())
		return sendReply(client, ERR_NOORIGIN, ":No origin specified");
	queueReply(client, std::string(":") + SERVER_NAME + " PONG " + SERVER_NAME + " :" + cmd.getParam(0));
}

// QUIT [:<reason>]
void Server::cmdQuit(Client& client, const Command& cmd)
{
	std::string reason = cmd.getParamCount() ? cmd.getParam(0) : "Client Quit";
	broadcastToPeers(client, ":" + client.getPrefix() + " QUIT :Quit: " + reason);
	queueReply(client, "ERROR :Closing Link: (Quit: " + reason + ")");
	disconnectClient(client, true);
}

// PASS <password>
void Server::cmdPass(Client& client, const Command& cmd)
{
	if (client.isRegistered())
		return sendReply(client, ERR_ALREADYREGISTERED, ":You may not reregister");
	if (cmd.getParamCount() < 1)
		return sendReply(client, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
	client.setPassOk(cmd.getParam(0) == _pswd);
	tryRegister(client);
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
	if (owner && owner != &client)
		return sendReply(client, ERR_NICKNAMEINUSE, nick + " :Nickname is already in use");
	if (nick == client.getNickname())
		return;

	if (client.isRegistered())
	{
		std::string msg = ":" + client.getPrefix() + " NICK :" + nick;
		client.setNickname(nick);
		broadcastToPeers(client, msg);
		return queueReply(client, msg);
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

	client.setUsername(cmd.getParam(0).substr(0, USER_MAX_LEN));
	client.setRealname(cmd.getParam(3));
	tryRegister(client);
}

void Server::cmdCap(Client& client, const Command& cmd)
{
	if (cmd.getParam(0) == "LS")
		queueReply(client, std::string(":") + SERVER_NAME + " CAP * LS :");
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
		handlers["JOIN"] = &Server::cmdJoin;
		handlers["PRIVMSG"] = &Server::cmdPrivmsg;
		handlers["KICK"] = &Server::cmdKick;
		handlers["INVITE"] = &Server::cmdInvite;
		handlers["TOPIC"] = &Server::cmdTopic;
		handlers["PING"] = &Server::cmdPing;
	}

	const std::string& name = cmd.getCommand();
	if (!client.isRegistered() && !allowedBeforeRegistration(name))
		return sendReply(client, ERR_NOTREGISTERED, ":You have not registered");

	std::map<std::string, CmdHandler>::iterator it = handlers.find(name);
	if (it == handlers.end())
		return sendReply(client, ERR_UNKNOWNCOMMAND, cmd.getRawCommand() + " :Unknown command");
	(this->*(it->second))(client, cmd);
}

// Completes registration once PASS, NICK and USER have all been received
void Server::tryRegister(Client& client)
{
	if (client.isRegistered() || !client.isPassGiven()
		|| client.getNickname().empty() || client.getUsername().empty())
		return;

	if (!client.isPassOk())
	{
		sendReply(client, ERR_PASSWDMISMATCH, ":Password incorrect");
		queueReply(client, "ERROR :Closing Link: (Password incorrect)");
		return disconnectClient(client, true);
	}

	client.setRegistered();

	sendReply(client, RPL_WELCOME, std::string(":Welcome to the ") + SERVER_NAME
		+ " Network, " + client.getPrefix());
	sendReply(client, RPL_YOURHOST, std::string(":Your host is ") + SERVER_NAME
		+ ", running version " + SERVER_VERSION);
	sendReply(client, RPL_CREATED, std::string(":This server was created today"));
	sendReply(client, RPL_MYINFO, std::string(SERVER_NAME) + " " + SERVER_VERSION + " o itkol kol");
}

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

		if (line.size() > Command::MAX_LINE_SIZE)
		{
			sendReply(*getClient(fd), ERR_INPUTTOOLONG, ":Input line was too long");
			continue;
		}

		Command cmd;
		if (!cmd.parse(line))
			continue;
		Client* client = getClient(fd);
		execCommand(*client, cmd);
		if (client->isClosing())
			return;
	}
}
