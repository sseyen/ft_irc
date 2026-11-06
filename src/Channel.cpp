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

bool Channel::hasClient(Client* client) const {
	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i] == client) {
			return true;
		}
	}
	return false;
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
		if (_invited[i] == client) {
			return true;
		}
	}
	return false;
}

void Channel::setTopic(std::string const& topic) {
	_topic = topic;
}

void Channel::setInviteOnly(bool value) {
	_invite_only = value;
}

void Channel::setTopicRestricted(bool value) {
	_topic_restricted = value;
}

void Channel::setKey(std::string const& key) {
	_key = key;
}

void Channel::setLimit(size_t limit) {
	_limit = limit;
}

void Channel::addClient(Client* client) {
	if (hasClient(client))
		return;
	_clients.push_back(client);
	removeInvite(client);
}

void Channel::removeClient(Client* client) {
	removeOperator(client);
	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i] == client) {
			_clients.erase(_clients.begin() + i);
			break;
		}
	}
}

void Channel::addOperator(Client* client) {
	if (isOperator(client))
		return;
	_operators.push_back(client);
}

void Channel::removeOperator(Client* client) {
	for (size_t i = 0; i < _operators.size(); i++) {
		if (_operators[i] == client) {
			_operators.erase(_operators.begin() + i);
			break;
		}
	}
}

void Channel::addInvite(Client* client) {
	if (isInvited(client))
		return;
	_invited.push_back(client);
}

void Channel::removeInvite(Client* client) {
	for (size_t i = 0; i < _invited.size(); i++) {
		if (_invited[i] == client) {
			_invited.erase(_invited.begin() + i);
			break;
		}
	}
}

std::string Channel::canJoin(Client* client, std::string const& key) const {
	if (isLimited() && getClients().size() >= getLimit())
		return "471"; // ERR_CHANNELISFULL
	if (isInviteOnly() && !isInvited(client))
		return "473"; // ERR_INVITEONLYCHAN
	if (isKeyProtected() && getKey() != key)
		return "475"; // ERR_BADCHANNELKEY

	return "";
}
