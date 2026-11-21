#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"
#include <set>

static std::string privmsgLine(const Client& sender, const std::string& target, const std::string& text)
{
	return ":" + sender.getPrefix() + " PRIVMSG " + target + " :" + text;
}

void Server::sendPrivmsg(Client& client, const std::string& target, const std::string& text)
{
	if (target[0] == '#' || target[0] == '&')
	{
		Channel* channel = findChannel(target);
		if (!channel)
			return sendReply(client, ERR_NOSUCHNICK, target + " :No such nick/channel");
		if (!channel->hasClient(&client))
			return sendReply(client, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
		return broadcast(*channel, privmsgLine(client, channel->getName(), text), &client);
	}

	Client* recipient = findClientByNick(target);
	if (!recipient || !recipient->isRegistered())
		return sendReply(client, ERR_NOSUCHNICK, target + " :No such nick/channel");
	queueReply(*recipient, privmsgLine(client, recipient->getNickname(), text));
}

// PRIVMSG <target>{,<target>} :<text>
void Server::cmdPrivmsg(Client& client, const Command& cmd)
{
	if (cmd.getParam(0).empty())
		return sendReply(client, ERR_NORECIPIENT, ":No recipient given (PRIVMSG)");
	if (cmd.getParam(1).empty())
		return sendReply(client, ERR_NOTEXTTOSEND, ":No text to send");

	std::vector<std::string> targets = splitList(cmd.getParam(0));
	std::set<std::string> seen;

	for (size_t i = 0; i < targets.size(); ++i)
	{
		// insert().second is false for a duplicate (case-insensitive): bob,BOB delivers once
		if (!targets[i].empty() && seen.insert(ircLower(targets[i])).second)
			sendPrivmsg(client, targets[i], cmd.getParam(1));
	}
}
