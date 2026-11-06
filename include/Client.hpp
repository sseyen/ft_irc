#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

class Channel;

class Client {
private:
	int _fd;
	std::string _nickname;
	std::string _username;
	std::string _hostname;
	std::string _realname;

	std::vector<Channel*> _channels;

	bool _registered;
	bool _passOk;

	Client& operator=(Client const& rhs);
	Client(Client const& other);

public:
	explicit Client(int fd);
	~Client();

	int getFd() const;
	const std::string& getNickname() const;
	const std::string& getUsername() const;
	const std::string& getHostname() const;
	const std::string& getRealname() const;
	std::string getPrefix() const;

	void setNickname(const std::string& nickname);
	void setUsername(const std::string& username);
	void setHostname(const std::string& hostname);
	void setRealname(const std::string& realname);
	void setPassOk(bool ok);
	void setRegistered();

	bool isPassOk() const;
	bool isRegistered() const;

	void addChannel(Channel* channel);
	void removeChannel(Channel* channel);
	const std::vector<Channel*>& getChannels() const;

};

#endif
