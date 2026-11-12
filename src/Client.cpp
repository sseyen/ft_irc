#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _registered(false), _passGiven(false), _passOk(false)
{

}

Client::Client(Client const& other)
{
	this->_fd = other._fd;
	this->_nickname = other._nickname;
	this->_username = other._username;
	this->_hostname = other._hostname;
	this->_realname = other._realname;
	this->_channels = other._channels;
	this->_registered = other._registered;
	this->_passGiven = other._passGiven;
	this->_passOk = other._passOk;
}

Client& Client::operator=(Client const& rhs)
{
	if (this != &rhs)
	{
		this->_fd = rhs._fd;
		this->_nickname = rhs._nickname;
		this->_username = rhs._username;
		this->_hostname = rhs._hostname;
		this->_realname = rhs._realname;
		this->_channels = rhs._channels;
		this->_registered = rhs._registered;
		this->_passGiven = rhs._passGiven;
		this->_passOk = rhs._passOk;
	}
	return *this;
}

Client::~Client()
{

}

int Client::getFd() const
{
	return _fd;
}

const std::string& Client::getNickname() const
{
	return _nickname;
}

const std::string& Client::getUsername() const
{
	return _username;
}

const std::string& Client::getHostname() const
{
	return _hostname;
}

void Client::setNickname(const std::string& nickname)
{
	_nickname = nickname;
}

void Client::setUsername(const std::string& username)
{
	_username = username;
}

void Client::setHostname(const std::string& hostname)
{
	_hostname = hostname;
}

const std::string& Client::getRealname() const
{
	return _realname;
}

// nick!user@host
std::string Client::getPrefix() const
{
	return _nickname + "!" + _username + "@" + (_hostname.empty() ? "localhost" : _hostname);
}

void Client::setRealname(const std::string& realname)
{
	_realname = realname;
}

void Client::setPassOk(bool ok)
{
	_passGiven = true;
	_passOk = ok;
}

void Client::setRegistered()
{
	_registered = true;
}

bool Client::isPassGiven() const
{
	return _passGiven;
}

bool Client::isPassOk() const
{
	return _passOk;
}

bool Client::isRegistered() const
{
	return _registered;
}

void Client::addChannel(Channel* channel)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (_channels[i] == channel)
			return;
	}
	_channels.push_back(channel);
}

void Client::removeChannel(Channel* channel)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (_channels[i] == channel)
		{
			_channels.erase(_channels.begin() + i);
			return;
		}
	}
}

const std::vector<Channel*>& Client::getChannels() const
{
	return _channels;
}
