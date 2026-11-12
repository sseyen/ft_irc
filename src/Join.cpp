#include "Server.hpp"
#include "Channel.hpp"
#include "Replies.hpp"

static const size_t CHANNEL_NAME_MAX_LEN = 50;
static const size_t MAX_CHANNELS_PER_CLIENT = 10;
static const size_t NAMES_MAX_LEN = 400;

//HELPERS/////////////////////////////////////////////////////

static bool isValidChannelName(const std::string& name)
{
	if (name.size() < 2 || name.size() > CHANNEL_NAME_MAX_LEN)
		return false;
	if (name[0] != '#' && name[0] != '&')
		return false;
	return name.find_first_of(" ,:\a") == std::string::npos;
}

std::vector<std::string> splitList(const std::string& list)
{
	std::vector<std::string> items;
	size_t start = 0;
	size_t comma;

	while ((comma = list.find(',', start)) != std::string::npos)
	{
		items.push_back(list.substr(start, comma - start));
		start = comma + 1;
	}
	items.push_back(list.substr(start));
	return items;
}

static std::string joinErrorText(const std::string& code)
{
	if (code == ERR_CHANNELISFULL)
		return ":Cannot join channel (+l)";
	if (code == ERR_INVITEONLYCHAN)
		return ":Cannot join channel (+i)";
	return ":Cannot join channel (+k)";
}

//JOIN////////////////////////////////////////////////////////

void Server::sendNamesList(Client& client, Channel& channel)
{
	const std::vector<Client*>& members = channel.getClients();
	const std::string& name = channel.getName();
	std::string names;

	for (size_t i = 0; i < members.size(); ++i)
	{
		if (!names.empty())
			names += " ";
		if (channel.isOperator(members[i]))
			names += "@";
		names += members[i]->getNickname();
		if (names.size() >= NAMES_MAX_LEN)
		{
			sendReply(client, RPL_NAMREPLY, "= " + name + " :" + names);
			names.clear();
		}
	}
	if (!names.empty())
		sendReply(client, RPL_NAMREPLY, "= " + name + " :" + names);
	sendReply(client, RPL_ENDOFNAMES, name + " :End of /NAMES list");
}

// JOIN 0
void Server::partAllChannels(Client& client)
{
	const std::vector<Channel*> channels = client.getChannels();

	for (size_t i = 0; i < channels.size(); ++i)
	{
		Channel* channel = channels[i];
		broadcast(*channel, ":" + client.getPrefix() + " PART " + channel->getName());
		channel->removeClient(&client);
		client.removeChannel(channel);
		if (channel->getClients().empty())
			removeChannel(channel->getName());
	}
}

void Server::joinChannel(Client& client, const std::string& name, const std::string& key)
{
	if (!isValidChannelName(name))
		return sendReply(client, ERR_BADCHANMASK, name + " :Bad Channel Mask");

	Channel* channel = findChannel(name);
	if (channel && channel->hasClient(&client))
		return;
	if (client.getChannels().size() >= MAX_CHANNELS_PER_CLIENT)
		return sendReply(client, ERR_TOOMANYCHANNELS, name + " :You have joined too many channels");

	if (channel)
	{
		std::string error = channel->canJoin(&client, key);
		if (!error.empty())
			return sendReply(client, error, name + " " + joinErrorText(error));
	}
	else
	{
		channel = createChannel(name);
		channel->addOperator(&client);
	}
	channel->addClient(&client);
	client.addChannel(channel);

	broadcast(*channel, ":" + client.getPrefix() + " JOIN " + channel->getName());
	if (!channel->getTopic().empty())
		sendTopic(client, *channel);
	sendNamesList(client, *channel);
}

// JOIN <channel>{,<channel>} [<key>{,<key>}]
// JOIN 0
void Server::cmdJoin(Client& client, const Command& cmd)
{
	if (cmd.getParamCount() < 1 || cmd.getParam(0).empty())
		return sendReply(client, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
	if (cmd.getParam(0) == "0")
		return partAllChannels(client);

	std::vector<std::string> names = splitList(cmd.getParam(0));
	std::vector<std::string> keys = splitList(cmd.getParam(1));

	for (size_t i = 0; i < names.size(); ++i)
	{
		if (!names[i].empty())
			joinChannel(client, names[i], i < keys.size() ? keys[i] : "");
	}
}
