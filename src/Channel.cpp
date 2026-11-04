#include "Channel.hpp"

Channel::Channel(std::string const& name)
	: _name(name), _invite_only(false), _topic_restricted(false), _limit(0) 
{}

Channel::~Channel() {

}

std::string const& Channel::getName() const {
	return _name;
}

std::string const& Channel::getTopic() const {
	return _topic;
}

std::vector<Client*> const& Channel::getClients() const {
	return _clients;
}

std::vector<Client*> const& Channel::getOperators() const {
	return _operators;
}

std::vector<Client*> const& Channel::getInvited() const {
	return _invited;
}

bool Channel::isInviteOnly() const {
	return _invite_only;
}

bool Channel::isTopicRestricted() const {
	return _topic_restricted;
}

std::string const& Channel::getKey() const {
	return _key;
}

bool Channel::isKeyProtected() const {
	return !(_key.empty());
}

size_t Channel::getLimit() const {
	return _limit;
}

bool Channel::isLimited() const {
	return (_limit > 0);
}

bool Channel::isOperator(Client* client) const {
	for (size_t i = 0; i < _operators.size(); i++) {
		if (client == _operators[i])
			return true;
	}
	return false;
}

bool Channel::isInvited(Client* client) const {
	for (size_t i = 0; i < _invited.size(); i++) {
		if (client == _invited[i])
			return true;
	}
	return false;
}
