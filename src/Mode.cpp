#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"
#include <sstream>

static bool modeKey(Channel* channel, const Command& cmd, size_t& arg, bool sign, std::string& params) {
	if (sign) {
		if (arg >= cmd.getParamCount())
			return false;
		channel->setKey(cmd.getParam(arg));
		params += " " + cmd.getParam(arg);
		arg++;
	}
	else
		channel->setKey("");
	return true;
}

static bool modeLimit(Channel* channel, const Command& cmd, size_t& arg, bool sign, std::string& params) {
	if (sign) {
		if (arg >= cmd.getParamCount())
			return false;
		int limit = std::atoi(cmd.getParam(arg).c_str());
		arg++;
		if (limit <= 0)
			return false;
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


void Server::cmdMode(Client& client, const Command& cmd) {
	if (cmd.getParamCount() < 1)
		return sendReply(client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
	std::string name = cmd.getParam(0);
	Channel* channel = findChannel(name);
	if (!channel)
		return sendReply(client, ERR_NOSUCHCHANNEL, name + " :No such channel");
	if (cmd.getParamCount() == 1)
		return sendReply(client, RPL_CHANNELMODEIS, name + " +");
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
		else if (c == 'i') {
			channel->setInviteOnly(sign);
			applied = true;
		}
		else if (c == 't') {
			channel->setTopicRestricted(sign);
			applied = true;
		}
		else if (c == 'k')
			applied = modeKey(channel, cmd, arg, sign, params);
		else if (c == 'l')
			applied = modeLimit(channel, cmd, arg, sign, params);
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
