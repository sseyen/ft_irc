#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"

static void modeKey(Channel* channel, const Command& cmd, size_t& arg, bool sign) {
	if (sign) {
		if (arg >= cmd.getParamCount())
			return;
		channel->setKey(cmd.getParam(arg));
		arg++;
	}
	else
		channel->setKey("");
}

static void modeLimit(Channel* channel, const Command& cmd, size_t& arg, bool sign) {
	if (sign) {
		if (arg >= cmd.getParamCount())
			return;
		int limit = std::atoi(cmd.getParam(arg).c_str());
		arg++;
		if (limit <= 0)
			return;
		channel->setLimit(limit);
	}
	else
		channel->setLimit(0);
}

void Server::modeOperator(Client& client, Channel* channel, const Command& cmd, size_t& arg, bool sign)
{
	if (arg >= cmd.getParamCount())
		return;
	std::string nick = cmd.getParam(arg);
	arg++;
	Client* target = findClientByNick(nick);
	if (!target) {
		sendReply(client, ERR_NOSUCHNICK, nick + " :No such nick/channel");
		return;
	}
	if (!channel->hasClient(target)) {
		sendReply(client, ERR_USERNOTINCHANNEL, nick + " " + channel->getName() + " :They aren't on that channel");
		return;
	}
	if (sign)
		channel->addOperator(target);
	else
		channel->removeOperator(target);
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
	for (size_t i = 0; i < modes.size(); i++) {
		char c = modes[i];
		if (c == '+' || c == '-')
			sign = c == '+';
		else if (c == 'i')
			channel->setInviteOnly(sign);
		else if (c == 't')
			channel->setTopicRestricted(sign);
		else if (c == 'k')
			modeKey(channel, cmd, arg, sign);
		else if (c == 'l')
			modeLimit(channel, cmd, arg, sign);
		else if (c == 'o')
			modeOperator(client, channel, cmd, arg, sign);
		else
			sendReply(client, ERR_UNKNOWNMODE, std::string(1, c) + " :is unknown mode char to me");
	}

}
