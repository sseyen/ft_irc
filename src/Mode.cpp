#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"
#include <sstream>
#include <cctype>

// Key: 1-23 printable chars, no spaces and no ',' (JOIN splits keys on ',')
static bool isValidKey(const std::string& key) {
	if (key.empty() || key.size() > 23)
		return false;
	for (size_t i = 0; i < key.size(); i++) {
		unsigned char c = key[i];
		if (c <= ' ' || c == ',' || c == 127)
			return false;
	}
	return true;
}

bool Server::modeKey(Client& client, Channel* channel, const Command& cmd, size_t& arg, bool sign, std::string& params) {
	if (sign) {
		if (arg >= cmd.getParamCount())
			return false;
		const std::string& key = cmd.getParam(arg);
		arg++;
		if (!isValidKey(key)) {
			sendReply(client, ERR_INVALIDKEY, channel->getName() + " :Key is not well-formed");
			return false;
		}
		channel->setKey(key);
		params += " " + key;
	}
	else {
		// -k <key>: clients send the key back, consume it so later modes keep their args
		if (arg < cmd.getParamCount())
			arg++;
		channel->setKey("");
		params += " *";
	}
	return true;
}

// Limit must be 1-5 digits and > 0, so it never overflows
static bool parseLimit(const std::string& str, size_t& limit) {
	if (str.empty() || str.size() > 5)
		return false;
	for (size_t i = 0; i < str.size(); i++)
		if (!std::isdigit(static_cast<unsigned char>(str[i])))
			return false;
	std::istringstream ss(str);
	ss >> limit;
	return limit > 0;
}

bool Server::modeLimit(Client& client, Channel* channel, const Command& cmd, size_t& arg, bool sign, std::string& params) {
	if (sign) {
		if (arg >= cmd.getParamCount())
			return false;
		const std::string& value = cmd.getParam(arg);
		arg++;
		size_t limit = 0;
		if (!parseLimit(value, limit)) {
			sendReply(client, ERR_INVALIDMODEPARAM, channel->getName() + " l " + value + " :Invalid limit");
			return false;
		}
		channel->setLimit(limit);
		std::ostringstream ss;
		ss << limit;
		params += " " + ss.str();
	}
	else
		channel->setLimit(0);
	return true;
}

// Appends mode char c to changes, adding the sign only when it differs from the last one
// Current channel modes for RPL_CHANNELMODEIS, e.g. "+itkl secret 5"
static std::string channelModes(Channel* channel, bool showKey) {
	std::string modes = "+";
	std::string params;
	if (channel->isInviteOnly())
		modes += 'i';
	if (channel->isTopicRestricted())
		modes += 't';
	if (channel->isKeyProtected()) {
		modes += 'k';
		if (showKey)
			params += " " + channel->getKey();
	}
	if (channel->isLimited()) {
		std::ostringstream ss;
		ss << channel->getLimit();
		modes += 'l';
		params += " " + ss.str();
	}
	return modes + params;
}

static void addChange(std::string& changes, char& lastSign, bool sign, char c) {
	char s = sign ? '+' : '-';
	if (s != lastSign) {
		changes += s;
		lastSign = s;
	}
	changes += c;
}

bool Server::modeOperator(Client& client, Channel* channel, const Command& cmd, size_t& arg, bool sign, std::string& params)
{
	if (arg >= cmd.getParamCount())
		return false;
	std::string nick = cmd.getParam(arg);
	arg++;
	Client* target = findClientByNick(nick);
	if (!target) {
		sendReply(client, ERR_NOSUCHNICK, nick + " :No such nick/channel");
		return false;
	}
	if (!channel->hasClient(target)) {
		sendReply(client, ERR_USERNOTINCHANNEL, nick + " " + channel->getName() + " :They aren't on that channel");
		return false;
	}
	if (sign)
		channel->addOperator(target);
	else
		channel->removeOperator(target);
	params += " " + target->getNickname();
	return true;
}


// MODE <nick> [<modes>]: no user modes are supported
void Server::modeUser(Client& client, const Command& cmd)
{
	const std::string& nick = cmd.getParam(0);
	Client* target = findClientByNick(nick);
	if (!target)
		return sendReply(client, ERR_NOSUCHNICK, nick + " :No such nick/channel");
	if (target != &client)
		return sendReply(client, ERR_USERSDONTMATCH, ":Cant change mode for other users");
	if (cmd.getParamCount() == 1)
		return sendReply(client, RPL_UMODEIS, "+");
	// +i/-i (sent by clients on connect) is ignored silently
	if (cmd.getParam(1).find_first_not_of("+-i") == std::string::npos)
		return;
	sendReply(client, ERR_UMODEUNKNOWNFLAG, ":Unknown MODE flag");
}

void Server::cmdMode(Client& client, const Command& cmd) {
	if (cmd.getParamCount() < 1 || cmd.getParam(0).empty())
		return sendReply(client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
	std::string name = cmd.getParam(0);
	if (name[0] != '#' && name[0] != '&')
		return modeUser(client, cmd);
	Channel* channel = findChannel(name);
	if (!channel)
		return sendReply(client, ERR_NOSUCHCHANNEL, name + " :No such channel");
	if (cmd.getParamCount() == 1)
		return sendReply(client, RPL_CHANNELMODEIS, channel->getName() + " "
			+ channelModes(channel, channel->hasClient(&client)));
	// Ban list query: bans are not supported, so the list is always empty
	if (cmd.getParamCount() == 2 && (cmd.getParam(1) == "b" || cmd.getParam(1) == "+b"))
		return sendReply(client, RPL_ENDOFBANLIST, name + " :End of channel ban list");
	if (!(channel->isOperator(&client)))
		return sendReply(client, ERR_CHANOPRIVSNEEDED, name + " :You're not channel operator");
	const std::string& modes = cmd.getParam(1);
	bool sign = true;
	size_t arg = 2;
	std::string changes;
	std::string params;
	char lastSign = 0;
	for (size_t i = 0; i < modes.size(); i++) {
		char c = modes[i];
		bool applied = false;
		if (c == '+' || c == '-') {
			sign = c == '+';
			continue;
		}
		// o always needs a nick; k and l need a value only when set
		bool needsArg = c == 'o' || (sign && (c == 'k' || c == 'l'));
		if (needsArg && arg >= cmd.getParamCount()) {
			sendReply(client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
			continue;
		}
		if (c == 'i') {
			channel->setInviteOnly(sign);
			applied = true;
		}
		else if (c == 't') {
			channel->setTopicRestricted(sign);
			applied = true;
		}
		else if (c == 'k')
			applied = modeKey(client, channel, cmd, arg, sign, params);
		else if (c == 'l')
			applied = modeLimit(client, channel, cmd, arg, sign, params);
		else if (c == 'o')
			applied = modeOperator(client, channel, cmd, arg, sign, params);
		else
			sendReply(client, ERR_UNKNOWNMODE, std::string(1, c) + " :is unknown mode char to me");
		if (applied)
			addChange(changes, lastSign, sign, c);
	}
	if (!changes.empty())
		broadcast(*channel, ":" + client.getPrefix() + " MODE " + channel->getName() + " " + changes + params);
}
