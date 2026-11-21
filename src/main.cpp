#include "Server.hpp"
#include <csignal>

static volatile sig_atomic_t running = 1;

int chckPort(char **argv) {
	int i = 0;
	while(argv[1][i] != '\0')
	{
		if (argv[1][i] < '0' || argv[1][i] > '9')
        {
            std::cout << "Error: Wrong port number\n";
            return -1;
        }
		i++;
	}
    int port_n = std::atoi(argv[1]);
    if (port_n < 0 || port_n > 65535)
	{
        std::cout << "Error: Wrong port number\n";
        return -1;
    }
    return port_n;
}

void handleSignal(int) {
	running = 0;
}

void setupSignals() {
	struct sigaction sa;

	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handleSignal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// Writing to a closed socket must not kill the server: send() returns -1 instead
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
}

int main(int argc, char **argv)
{
	std::cout << "   ____                            _     _   _  ___ _____   _   _                            _        _   \n";
	std::cout << "  / ___|___  _ __  _ __   ___  ___| |_  | \\ | |/ _ \\_   _| | |_| |__  _   _   ___  ___   ___| | _____| |_ \n";
	std::cout << " | |   / _ \\| '_ \\| '_ \\ / _ \\/ __| __| |  \\| | | | || |   | __| '_ \\| | | | / __|/ _ \\ / __| |/ / _ \\ __|\n";
	std::cout << " | |__| (_) | | | | | | |  __/ (__| |_  | |\\  | |_| || |   | |_| | | | |_| | \\__ \\ (_) | (__|   <  __/ |_ \n";
	std::cout << "  \\____\\___/|_| |_|_| |_|\\___|\\___|\\__| |_| \\_|\\___/ |_|    \\__|_| |_|\\__, | |___/\\___/ \\___|_|\\_\\___|\\__|\n";
	std::cout << "                                                                      |___/                               \n";

	if(argc != 3)
	{
		std::cerr << "We accept thy 3 arguments here!\n";
		return 1;
	}
	int port_n = chckPort(argv);
	if(port_n == -1)
		return 1;
	setupSignals();
	try
	{
		Server serv(port_n, argv[2]);
		struct pollfd *serv_poll = serv.getPoll();
		int ready;
		while(running)
		{
			// Checks server until something happens
			ready = poll(serv_poll, MAX_FDS, -1);
			if(ready == -1)
			{
				if(errno == EINTR)
					continue;
				std::cerr << "Error: " << std::strerror(errno) << '\n';
				break;
			}
			if(serv_poll[0].revents & POLLIN)
			{
				// Socket accepted to the server
				if (serv.cnctClient())
					std::cout << "Socket accepted\n";
			}
			// Check all connections
			for(int i = 1; i < MAX_FDS; i++)
			{
				short revents = serv_poll[i].revents;
				// Socket error / invalid fd: nothing to read, drop it
				if (revents & (POLLERR | POLLNVAL))
				{
					serv.dropClient(i);
					continue;
				}
				// Incoming data or hangup (recv() then returns 0 -> dropClient)
				if (revents & (POLLIN | POLLHUP))
					serv.recvMsg(i);
				if (serv_poll[i].fd == -1)	// recvMsg disconnected the client
					continue;
				// Socket writable: flush queued replies
				if (revents & POLLOUT)
					serv.sendMsg(i);
			}
		}
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what();
		return 1;
	}
	return 0;
}
