#include "Server.hpp"

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

/*** @brief Main function with the main loop.*/
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
	Server serv(port_n, argv[2]);
	struct pollfd *serv_poll = serv.getPoll();
	int ready;
	while(true)
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
		if(ready > 0)
		{
			if(serv_poll[0].revents & POLLIN)
			{
				// Client accepted to the server
        		if (serv.cnctClient())
					std::cout << "Deus Vult! (client accepted)\n";
			}
			// Check all connectioins
			for(int i = 1; i < MAX_FDS; i++)
			{
				// Skip invalid fds
				if (serv_poll[i].fd == -1)
					continue;
				// Incoming data received or hangup
				if (serv_poll[i].revents & (POLLIN | POLLHUP))
					serv.recvMsg(i);
			}
		}
	}
	return 0;
}
 
   
   
   
    