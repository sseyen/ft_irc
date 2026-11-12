#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"
#include <sstream>

static const size_t TOPIC_MAX_LEN = 307;

void Server::sendTopic(Client& client, Channel& channel)
{
	const std::string& chan_name = channel.getName();

	if (channel.getTopic().empty())
		return sendReply(client, RPL_NOTOPIC, chan_name + " :No topic is set");

	std::ostringstream when;
	when << channel.getTopicTime();
	sendReply(client, RPL_TOPIC, chan_name + " :" + channel.getTopic());
	sendReply(client, RPL_TOPICWHOTIME, chan_name + " " + channel.getTopicSetter() + " " + when.str());
}

// TOPIC <channel> [<topic>]
// "TOPIC #c" -> check topic, "TOPIC #c :" -> clear topic, "TOPIC #c :text" -> set topic
void Server::cmdTopic(Client& client, const Command& cmd)
{
	if (cmd.getParamCount() < 1 || cmd.getParam(0).empty())
		return sendReply(client, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");

	const std::string& chan_name = cmd.getParam(0);
	Channel* channel = findChannel(chan_name);
	if (!channel)
		return sendReply(client, ERR_NOSUCHCHANNEL, chan_name + " :No such channel");
	if (!channel->hasClient(&client))
		return sendReply(client, ERR_NOTONCHANNEL, channel->getName() + " :You're not on that channel");

	if (cmd.getParamCount() < 2)
		return sendTopic(client, *channel);

	if (channel->isTopicRestricted() && !channel->isOperator(&client))
		return sendReply(client, ERR_CHANOPRIVSNEEDED, channel->getName() + " :You're not channel operator");

	std::string topic = cmd.getParam(1).substr(0, TOPIC_MAX_LEN);
	if (topic.empty() && channel->getTopic().empty())
		return;
	channel->setTopic(topic, client.getNickname());
	broadcast(*channel, ":" + client.getPrefix() + " TOPIC " + channel->getName() + " :" + topic);
}
