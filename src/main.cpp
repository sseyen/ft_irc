#include "../include/Server.hpp"

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
	int port_n = std::atoi(argv[1]);
	Server serv(port_n, argv[2]);
	struct pollfd *serv_poll = serv.getPoll();
	int ready;
	while(true)
	{
		ready = poll(serv_poll, MAX_FDS, 500);
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
        		if (serv.cnctClient())
					std::cout << "Deus Vult! (client accepted)\n";
			}
			else
			{
				for(int i = 1; i < MAX_FDS; i++)
				{
					if (serv_poll[i].fd == -1)
					{
						continue;
					}
					if (serv_poll[i].revents & (POLLIN | POLLHUP))
						serv.recvMsg(i);
				}
			}
		}
	}
	return 0;
}
 
   
   
   
    