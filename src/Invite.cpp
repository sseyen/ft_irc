#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"

// INVITE <nickname> <channel>
void Server::cmdInvite(Client& client, const Command& cmd)
{
	if (cmd.getParamCount() < 2 || cmd.getParam(0).empty() || cmd.getParam(1).empty())
		return sendReply(client, ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");

	const std::string& nick = cmd.getParam(0);
	const std::string& name = cmd.getParam(1);

	Client* target = findClientByNick(nick);
	if (!target || !target->isRegistered())
		return sendReply(client, ERR_NOSUCHNICK, nick + " :No such nick/channel");

	Channel* channel = findChannel(name);
	if (!channel)
		return sendReply(client, ERR_NOSUCHCHANNEL, name + " :No such channel");

	const std::string& chan = channel->getName();

	if (!channel->hasClient(&client))
		return sendReply(client, ERR_NOTONCHANNEL, chan + " :You're not on that channel");

	if (channel->isInviteOnly() && !channel->isOperator(&client))
		return sendReply(client, ERR_CHANOPRIVSNEEDED, chan + " :You're not channel operator");

	if (channel->hasClient(target))
		return sendReply(client, ERR_USERONCHANNEL, target->getNickname() + " " + chan + " :is already on channel");

	channel->addInvite(target);
	sendReply(client, RPL_INVITING, target->getNickname() + " " + chan);
	sendRaw(*target, ":" + client.getPrefix() + " INVITE " + target->getNickname() + " " + chan);
}
