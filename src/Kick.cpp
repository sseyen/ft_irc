#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"

static const size_t KICK_MAX_LEN = 255;

void Server::kickUser(Client& client, const std::string& chanName, const std::string& nick, const std::string& comment)
{
	Channel* channel = findChannel(chanName);
	if (!channel)
		return sendReply(client, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
	const std::string name = channel->getName();
	if (!channel->hasClient(&client))
		return sendReply(client, ERR_NOTONCHANNEL, name + " :You're not on that channel");
	if (!channel->isOperator(&client))
		return sendReply(client, ERR_CHANOPRIVSNEEDED, name + " :You're not channel operator");

	Client* target = findClientByNick(nick);
	if (!target || !target->isRegistered())
		return sendReply(client, ERR_NOSUCHNICK, nick + " :No such nick/channel");
	if (!channel->hasClient(target))
		return sendReply(client, ERR_USERNOTINCHANNEL, target->getNickname() + " " + name + " :They aren't on that channel");

	broadcast(*channel, ":" + client.getPrefix() + " KICK " + name + " " + target->getNickname() + " :" + comment);
	channel->removeClient(target);
	target->removeChannel(channel);
	if (channel->getClients().empty())
		removeChannel(name);
}

// KICK <channel> <user>{,<user>} [<comment>]
// RFC 2812 allows <channel>,<channel> <user>,<user>
// KICK messages relayed SHOULD be sent separaretely for each channel/user pair
void Server::cmdKick(Client& client, const Command& cmd)
{
	if (cmd.getParamCount() < 2 || cmd.getParam(0).empty() || cmd.getParam(1).empty())
		return sendReply(client, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");

	std::vector<std::string> channels = splitList(cmd.getParam(0));
	std::vector<std::string> users = splitList(cmd.getParam(1));
	if (channels.size() != 1 && channels.size() != users.size())
		return sendReply(client, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");

	std::string comment = cmd.getParam(2);
	if (comment.empty())
		comment = client.getNickname();
	comment = comment.substr(0, KICK_MAX_LEN);

	for (size_t i = 0; i < users.size(); ++i)
	{
		const std::string& chanName = channels.size() == 1 ? channels[0] : channels[i];
		if (!chanName.empty() && !users[i].empty())
			kickUser(client, chanName, users[i], comment);
	}
}
