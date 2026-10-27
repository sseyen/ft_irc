*This project has been created as part of the 42 curriculum by alisseye, gvasylie.*

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

## Resources

- [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459) — Internet Relay Chat Protocol
- [RFC 2812](https://www.rfc-editor.org/rfc/rfc2812) — Internet Relay Chat: Client Protocol
- AI: an assistant drafted the Makefile and this README. No server code was generated.
