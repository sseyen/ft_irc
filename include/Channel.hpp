#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <cstddef>
#include <vector>
#include <string>
#include <ctime>

class Client;

class Channel {
private:
	std::string _name;
	std::string _topic;
	std::string _topicSetter;
	time_t _topicTime;

	std::vector<Client*> _clients;
	std::vector<Client*> _operators;
	std::vector<Client*> _invited;

	bool _invite_only;
	bool _topic_restricted;
	std::string _key;
	size_t _limit;
	
	Channel& operator=(Channel const& rhs);
	Channel(Channel const& other);
	
public:
	Channel(std::string const& name);
	~Channel();

	std::string const& getName() const;
	std::string const& getTopic() const;
	std::string const& getTopicSetter() const;
	time_t getTopicTime() const;

	std::vector<Client*> const& getClients() const;
	std::vector<Client*> const& getOperators() const;
	std::vector<Client*> const& getInvited() const;

	bool isInviteOnly() const;
	bool isTopicRestricted() const;
	std::string const& getKey() const;
	bool isKeyProtected() const;
	size_t getLimit() const;
	bool isLimited() const;

	bool hasClient(Client* client) const;
	bool isOperator(Client* client) const;
	bool isInvited(Client* client) const;

	void setTopic(std::string const& topic, std::string const& setter);
	void setInviteOnly(bool value);
	void setTopicRestricted(bool value);
	void setKey(std::string const& key);
	void setLimit(size_t limit);

	void addClient(Client* client);
	void removeClient(Client* client);
	void addOperator(Client* client);
	void removeOperator(Client* client);
	void addInvite(Client* client);
	void removeInvite(Client* client);

	std::string canJoin(Client* client, std::string const& key) const;

};

#endif