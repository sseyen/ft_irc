#ifndef REPLIES_HPP
#define REPLIES_HPP

//
// Replies
//

// Registration (PASS / NICK / USER)
#define RPL_WELCOME				"001"	// :Welcome to the <networkname> Network, <nick>[!<user>@<host>]
#define RPL_YOURHOST			"002"	// :Your host is <servername>, running version <version>
#define RPL_CREATED				"003"	// :This server was created <datetime>
#define RPL_MYINFO				"004"	// <servername> <version> <user modes> <channel modes> <channel modes with a param>

// Channels: JOIN / PART / NAMES
#define RPL_NAMREPLY			"353"	// <symbol> <channel> :[prefix]<nick>{ [prefix]<nick>}   symbol: = public, @ secret, * private
#define RPL_ENDOFNAMES			"366"	// <channel> :End of /NAMES list

// Channels: TOPIC
#define RPL_NOTOPIC				"331"	// <channel> :No topic is set
#define RPL_TOPIC				"332"	// <channel> :<topic>
#define RPL_TOPICWHOTIME		"333"	// <channel> <nick> <setat unix time>

// Channels: INVITE / KICK
#define RPL_INVITING			"341"	// <nick> <channel>

// Modes (MODE i t k o l)
#define RPL_UMODEIS				"221"	// <user modes>
#define RPL_CHANNELMODEIS		"324"	// <channel> <modestring> <mode arguments>...
#define RPL_CREATIONTIME		"329"	// <channel> <creation unix time>

// WHO
#define RPL_WHOREPLY			"352"	// <channel> <user> <host> <server> <nick> <flags> :<hopcount> <realname>   flags: H here, G away, @ op
#define RPL_ENDOFWHO			"315"	// <mask> :End of WHO list

//
// Errors
//

// Registration (PASS / NICK / USER)
#define ERR_NOMOTD				"422"	// :MOTD File is missing
#define ERR_NOTREGISTERED		"451"	// :You have not registered
#define ERR_NEEDMOREPARAMS		"461"	// <command> :Not enough parameters
#define ERR_ALREADYREGISTERED	"462"	// :You may not reregister
#define ERR_PASSWDMISMATCH		"464"	// :Password incorrect

// Nicknames (NICK)
#define ERR_NONICKNAMEGIVEN		"431"	// :No nickname given
#define ERR_ERRONEUSNICKNAME	"432"	// <nick> :Erroneus nickname
#define ERR_NICKNAMEINUSE		"433"	// <nick> :Nickname is already in use

// Generic command errors
#define ERR_UNKNOWNCOMMAND		"421"	// <command> :Unknown command
#define ERR_INPUTTOOLONG		"417"	// :Input line was too long
#define ERR_NOORIGIN			"409"	// :No origin specified

// Messaging (PRIVMSG / NOTICE)
#define ERR_NOSUCHNICK			"401"	// <nick> :No such nick/channel
#define ERR_CANNOTSENDTOCHAN	"404"	// <channel> :Cannot send to channel
#define ERR_NORECIPIENT			"411"	// :No recipient given (<command>)
#define ERR_NOTEXTTOSEND		"412"	// :No text to send

// Channels: JOIN / PART / NAMES
#define ERR_NOSUCHCHANNEL		"403"	// <channel> :No such channel
#define ERR_TOOMANYCHANNELS		"405"	// <channel> :You have joined too many channels
#define ERR_BADCHANMASK			"476"	// <channel> :Bad Channel Mask
#define ERR_NOTONCHANNEL		"442"	// <channel> :You're not on that channel
#define ERR_CHANNELISFULL		"471"	// <channel> :Cannot join channel (+l)
#define ERR_INVITEONLYCHAN		"473"	// <channel> :Cannot join channel (+i)
#define ERR_BADCHANNELKEY		"475"	// <channel> :Cannot join channel (+k)

// Channels: INVITE / KICK
#define ERR_USERNOTINCHANNEL	"441"	// <nick> <channel> :They aren't on that channel
#define ERR_USERONCHANNEL		"443"	// <nick> <channel> :is already on channel
#define ERR_CHANOPRIVSNEEDED	"482"	// <channel> :You're not channel operator

// Modes (MODE i t k o l)
#define ERR_UNKNOWNMODE			"472"	// <modechar> :is unknown mode char to me
#define ERR_UMODEUNKNOWNFLAG	"501"	// :Unknown MODE flag
#define ERR_USERSDONTMATCH		"502"	// :Cant change mode for other users
#define ERR_INVALIDKEY			"525"	// <channel> :Key is not well-formed
#define ERR_INVALIDMODEPARAM	"696"	// <target> <modechar> <parameter> :<description>

#endif
