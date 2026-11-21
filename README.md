*This project has been created as part of the 42 curriculum by alisseye, gvasylie, pibouill*

# ft_irc

## Description

ft_irc is an IRC server written in C++98. A regular IRC client connects to it, authenticates with a password, registers a nickname and a username, and joins channels. Messages sent to a channel are forwarded to every other member. Channel operators can kick, invite, set the topic, and change the channel mode.

## Instructions

`c++` and `make` are enough. No external libraries.

```
make
./ircserv <port> <password>
```

`port` is the listening port. `password` is required to connect. `make clean` removes `obj/`. `make fclean` also removes `ircserv`. `make re` rebuilds from scratch.

## Reference client

Our reference client is [irssi](https://irssi.org/). Every feature of the server was tested with it.

`nc` also works for raw testing. Use `nc -C` so each line ends with `\r\n`, as the IRC protocol requires.

## Usage examples


Start the server:

```
./ircserv 6667 secret
```

### With nc

```
$ nc -C 127.0.0.1 6667
PASS secret
NICK alice
USER alice 0 * :Alice
JOIN #42
PRIVMSG #42 :hello everyone
```

### With irssi

```
irssi -c 127.0.0.1 -p 6667 -w secret -n alice
/join #42
/msg bob hi bob
/mode #42 +i
```

## Resources

- [IRC documentation](https://modern.ircdocs.horse/) — Modern IRC Client Protocol
- [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459) — Internet Relay Chat Protocol
- [RFC 2812](https://www.rfc-editor.org/rfc/rfc2812) — Internet Relay Chat: Client Protocol

- AI usage: an assistant drafted the Makefile and this README. No server code was generated.
